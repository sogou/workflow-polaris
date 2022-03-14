#include <set>
#include "workflow/StringUtil.h"
#include "PolarisPolicies.h"

namespace polaris {

static constexpr char const *META_LABLE_EXACT = "EXACT";
static constexpr char const *META_LABLE_REGEX = "REGEX";

static inline bool meta_lable_equal(const struct meta_label& meta,
									const std::string& str)
{
	if (meta.value == str)
		return true;

	//if (meta.type == META_LABLE_REGEX)
	//TODO support REGEX

	return false;
}

PolarisPolicyConfig::PolarisPolicyConfig(const std::string& policy_name,
										 const PolarisConfig& conf) :
	policy_name(policy_name)
{
	this->enable_rule_base_router = false;
	this->enable_dst_meta_router = false;
	this->enable_nearby_based_router = false;
	this->failover_type = MetadataFailoverNone;

	std::vector<std::string> router_chain = conf.get_service_router_chain();
	for (auto router : router_chain)
	{
		if (router == "ruleBasedRouter")
			this->set_rule_base_router(true);
		else if (router == "dstMetaRouter")
			this->set_dst_meta_router(true);
		else if (router == "nearbyBasedRouter")
		{
			this->set_nearby_based_router(true,
										  conf.get_nearby_match_level(),
										  conf.get_nearby_max_match_level(),
										  conf.get_nearby_unhealthy_degrade() == true ?
										  conf.get_nearby_unhealthy_degrade_percent() : 0,
										  conf.get_nearby_enable_recover_all(),
										  true /* strict nearby */);
		}
	}

	if (conf.get_api_location_region() != "unknown")
		this->location_region = conf.get_api_location_region();
	if (conf.get_api_location_zone() != "unknown")
		this->location_zone = conf.get_api_location_zone();
	if (conf.get_api_location_campus() != "unknown")
		this->location_campus = conf.get_api_location_campus();
}

void PolarisPolicyConfig::set_nearby_based_router(bool flag,
												 std::string match_level,
												 std::string max_match_level,
												 short percentage,
												 bool enable_recover_all,
												 bool strict_nearby)
{
	this->enable_nearby_based_router = flag;

	if (match_level == "zone")
		this->nearby_match_level = NearbyMatchLevelZone;
	else if (match_level == "campus")
		this->nearby_match_level = NearbyMatchLevelCampus;
	else if (match_level == "region")
		this->nearby_match_level = NearbyMatchLevelRegion;

	if (max_match_level == "zone")
		this->nearby_max_match_level = NearbyMatchLevelZone;
	if (max_match_level == "campus")
		this->nearby_max_match_level = NearbyMatchLevelCampus;
	else if (max_match_level == "region")
		this->nearby_max_match_level = NearbyMatchLevelRegion;

	this->nearby_unhealthy_percentage = percentage;
	this->nearby_enable_recover_all = enable_recover_all;
	this->strict_nearby = strict_nearby;
}

PolarisInstanceParams::PolarisInstanceParams(const struct instance *inst,
											 const AddressParams *params) :
	PolicyAddrParams(params),
	logic_set(inst->logic_set),
	service_namespace(inst->service_namespace),
	metadata(inst->metadata)
{
	this->priority = inst->priority;
	this->enable_healthcheck = inst->enable_healthcheck;
	this->healthy = inst->healthy;
	this->isolate = inst->isolate;

	this->weight = inst->weight;
	// params.weight will not affect here
	// this->weight == 0 has special meaning
}

PolarisPolicy::PolarisPolicy(const PolarisPolicyConfig *config) :
	config(*config),
	inbound_rwlock(PTHREAD_RWLOCK_INITIALIZER),
	outbound_rwlock(PTHREAD_RWLOCK_INITIALIZER)
{
	this->total_weight = 0;
	this->available_weight = 0;
}

void PolarisPolicy::update_instances(const std::vector<struct instance>& instances)
{
	std::vector<EndpointAddress *> addrs;
	EndpointAddress *addr;
	std::string name;

	AddressParams params = ADDRESS_PARAMS_DEFAULT;

	for (size_t i = 0; i < instances.size(); i++)
	{
		name = instances[i].host + ":" + std::to_string(instances[i].port);
		printf("update_instances() %s:%d region=%s zone=%s campus=%s\n",
				instances[i].host.c_str(), instances[i].port,
				instances[i].region.c_str(), instances[i].zone.c_str(), instances[i].campus.c_str());
		addr = new EndpointAddress(name,
						new PolarisInstanceParams(&instances[i], &params));
		addrs.push_back(addr);
	}

	pthread_rwlock_wrlock(&this->rwlock);
	this->clear_instances_locked();

	for (size_t i = 0; i < addrs.size(); i++)
		this->add_server_locked(addrs[i]);
	pthread_rwlock_unlock(&this->rwlock);
}

void PolarisPolicy::clear_instances_locked()
{
	for (EndpointAddress *addr : this->servers)
	{
		this->server_list_change(addr, REMOVE_SERVER);
		if (--addr->ref == 0)
		{
			this->pre_delete_server(addr);
			delete addr;
		}
	}

	this->servers.clear();
	this->server_map.clear();
	this->nalives = 0;
	this->total_weight = 0;
	this->available_weight = 0;
}

void PolarisPolicy::add_server_locked(EndpointAddress *addr)
{
	this->server_map[addr->address].push_back(addr);
	this->servers.push_back(addr);
	this->recover_one_server(addr);
	this->server_list_change(addr, ADD_SERVER);

	PolarisInstanceParams *params = static_cast<PolarisInstanceParams *>(addr->params);
	this->total_weight += params->get_weight();
	// TODO: depends on what we need in select()
}

void PolarisPolicy::recover_one_server(const EndpointAddress *addr)
{
	this->nalives++;
	PolarisInstanceParams *params = static_cast<PolarisInstanceParams *>(addr->params);
	this->available_weight += params->get_weight();
}

void PolarisPolicy::fuse_one_server(const EndpointAddress *addr)
{
	this->nalives--;
	PolarisInstanceParams *params = static_cast<PolarisInstanceParams *>(addr->params);
	this->available_weight -= params->get_weight();
}

void PolarisPolicy::update_inbounds(const std::vector<struct routing_bound>& inbounds)
{
	std::set<std::string> cleared_set;
	BoundRulesMap::iterator it;

	pthread_rwlock_wrlock(&this->inbound_rwlock);
	for (size_t i = 0; i < inbounds.size(); i++)
	{
		std::string src_name;

		if (inbounds[i].source_bounds.size())
			src_name = inbounds[i].source_bounds[0].service;

		// clear the previous
		if (cleared_set.find(src_name) == cleared_set.end())
		{
			it = this->inbound_rules.find(src_name);

			if (it != this->inbound_rules.end())
				it->second.clear();
			else
				this->inbound_rules[src_name]; // construct vector

			cleared_set.insert(src_name);
		}

		this->inbound_rules[src_name].push_back(inbounds[i]);
	}

	pthread_rwlock_unlock(&this->inbound_rwlock);
}

void PolarisPolicy::update_outbounds(const std::vector<struct routing_bound>& outbounds)
{
	std::set<std::string> cleared_set;
	BoundRulesMap::iterator it;

	pthread_rwlock_wrlock(&this->outbound_rwlock);
	for (size_t i = 0; i < outbounds.size(); i++)
	{
		std::string src_name;

		if (outbounds[i].source_bounds.size())
			src_name = outbounds[i].source_bounds[0].service;

		// clear the previous
		if (cleared_set.find(src_name) == cleared_set.end())
		{
			it = this->outbound_rules.find(src_name);

			if (it != this->inbound_rules.end())
				it->second.clear();
			else
				this->outbound_rules[src_name]; // construct vector

			cleared_set.insert(src_name);
		}

		this->outbound_rules[src_name].push_back(outbounds[i]);
	}

	pthread_rwlock_unlock(&this->outbound_rwlock);
}

bool PolarisPolicy::select(const ParsedURI& uri, WFNSTracing *tracing,
						   EndpointAddress **addr)
{
	std::vector<struct destination_bound> *dst_bounds = NULL;
	std::vector<EndpointAddress *> matched_subset;
	std::string caller_name;
	std::string caller_namespace;
	std::map<std::string, std::string> meta;
	bool ret = true;

	this->check_breaker();

	if (!this->split_fragment(uri.fragment, caller_name, caller_namespace, meta))
		return false;

	if (meta.size())
	{
		// will be refactored as chain mode
		if (this->config.enable_rule_base_router)
		{
			this->matching_bounds(caller_name, caller_namespace, meta, &dst_bounds);
			if (dst_bounds && dst_bounds->size())
			{
				pthread_rwlock_rdlock(&this->rwlock);
				ret = this->matching_subset(dst_bounds, matched_subset);
				pthread_rwlock_unlock(&this->rwlock);
			}
		}
		else if (this->config.enable_dst_meta_router)
		{
			pthread_rwlock_rdlock(&this->rwlock);
			ret = this->matching_meta(meta, matched_subset);
			pthread_rwlock_unlock(&this->rwlock);
		}
	}

	if (ret)
	{
		pthread_rwlock_rdlock(&this->rwlock);
		if (matched_subset.size())
		{
			*addr = this->get_one(matched_subset, tracing);

			for (size_t i = 0; i < matched_subset.size(); i++)
			{
				if (*addr != matched_subset[i])
				{
					if (--matched_subset[i]->ref == 0)
					{
						this->pre_delete_server(matched_subset[i]);
						delete matched_subset[i];
					}
				}
			}
		}
		else if (this->servers.size())
		{
			*addr = this->get_one(this->servers, tracing);
			++(*addr)->ref;
		}
		else
			ret = false;

		pthread_rwlock_unlock(&this->rwlock);
	}

	return ret;
}

/*
 *	One "caller->callee" pair may has multiple in/out routing_bound.
 *	One routing_bound guarantees to consist of one src in the vector.
 *	Here will get the first matched src`s dst vector.
 *	If the chosen dsts` subsets are all unhealthy, maching_bounds doesn`t care.
*/
void PolarisPolicy::matching_bounds(
					const std::string& caller_name,
					const std::string& caller_namespace,
					const std::map<std::string, std::string>& meta,
					std::vector<struct destination_bound> **dst_bounds)
{
	std::vector<struct destination_bound> *dst = NULL;

	pthread_rwlock_t *lock = &this->inbound_rwlock;
	pthread_rwlock_rdlock(lock);

	BoundRulesMap& rules = this->inbound_rules;
	BoundRulesMap::iterator iter = this->inbound_rules.find(caller_name);

	if (iter == this->inbound_rules.end() &&
		this->inbound_rules.find("*") == this->inbound_rules.end())
	{
		pthread_rwlock_unlock(lock);
		lock = &this->outbound_rwlock;
		pthread_rwlock_rdlock(lock);
		rules = this->outbound_rules;
	}

	iter = rules.find(caller_name);
	if (iter == rules.end())
		iter = rules.find("*");

	if (iter != rules.end())
	{
		for (struct routing_bound& rule : iter->second)
		{
			if (this->matching_rules(caller_name, caller_namespace,
									 meta, rule.source_bounds))
			{
				dst = &rule.destination_bounds;
				break;
			}
		}

		if (dst)
			*dst_bounds = dst;
	}

	pthread_rwlock_unlock(lock);
}

/*
 * One subset vector is the instances matched one dst_bound.
 * Return the healthy subset whose matched dst_bound has top priority.
 * If multiple healthy dst_bounds have the same priority,
 * select one bound randomly according to their weight and return its subset.
 * If no instancs is matched by any dst_bounds, return false.
 */
bool PolarisPolicy::matching_subset(
			std::vector<struct destination_bound> *dst_bounds,
			std::vector<EndpointAddress *>& matched_subset)
{
	std::map<int, std::vector<struct destination_bound *>> bound_map;
	std::map<int, std::vector<struct destination_bound *>>::iterator it;
	std::vector<std::vector<EndpointAddress *>> top_subsets;
	std::vector<std::vector<EndpointAddress *>> cur_subsets;
	std::vector<std::vector<EndpointAddress *>>& subsets = top_subsets;
	bool found = false;
	size_t i;

	for (i = 0; i < dst_bounds->size(); i++)
		bound_map[(*dst_bounds)[i].priority].push_back(&((*dst_bounds)[i]));

	for (it = bound_map.begin(); it != bound_map.end() && !found; it++)
	{
		if (top_subsets.size())
		{
			cur_subsets.clear();
			subsets = cur_subsets;
		}

		subsets.resize(it->second.size());
		for (i = 0; i < it->second.size(); i++)
		{
			if (this->matching_instances(it->second[i], subsets[i]))
				found = true;
		}

		if (found == true)
			break;
	}

	if (found == false)
	{
		if (top_subsets.size())
		{
			subsets = top_subsets;
			it = bound_map.begin();
		}
		else
			return false;
	}

	if (subsets.size() == 1)
		i = 0;
	else // should move
		i = this->subsets_weighted_random(it->second, subsets);

	for (size_t j = 0; j < subsets[i].size(); j++)
	{
		++subsets[i][j]->ref;
		matched_subset.push_back(subsets[i][j]);
	}

	return true;
}

size_t PolarisPolicy::subsets_weighted_random(
					const std::vector<struct destination_bound *>& bounds,
					const std::vector<std::vector<EndpointAddress *>>& subsets)
{
	int x, s = 0;
	int total_weight = 0;
	int available_bounds_count = 0;
	size_t i;

	for (i = 0; i < bounds.size(); i++)
	{
		if (subsets[i].size())
		{
			total_weight += bounds[i]->weight;
			available_bounds_count++;
		}
	}

	if (total_weight == 0)
		total_weight = available_bounds_count;

	x = rand() % total_weight;

	for (i = 0; i < bounds.size(); i++)
	{
		if (subsets[i].size() == 0)
			continue;

		s += bounds[i]->weight;
		if (s > x)
			break;
	}

	if (i == bounds.size())
	{
		do {
			i--;
		} while (subsets[i].size() == 0);
	}

	return i;
}

bool PolarisPolicy::matching_instances(struct destination_bound *dst_bounds,
									   std::vector<EndpointAddress *>& subset)
{
	// fill all servers which match all the meta in dst_bounds
	// no matter they are heathy or not
	// if no healty instances : return false; else : return true;
	PolarisInstanceParams *params;
	bool flag;

	for (size_t i = 0; i < this->servers.size(); i++)
	{
		params = static_cast<PolarisInstanceParams *>(this->servers[i]->params);

		if (dst_bounds->service_namespace != params->get_namespace())
			continue;

		const std::map<std::string, std::string>& inst_meta = params->get_meta();
		flag = true;

		for (const auto &bound_meta : dst_bounds->metadata)
		{
			const auto inst_meta_it = inst_meta.find(bound_meta.first);

			if (inst_meta_it == inst_meta.end() ||
				(bound_meta.second.value != "*" &&
				!meta_lable_equal(bound_meta.second, inst_meta_it->second)))
			{
				flag = false;
				break;
			}
		}

		if (flag == true)
			subset.push_back(this->servers[i]);
	}

	return true;
}

bool PolarisPolicy::matching_rules(
					const std::string& caller_name,
					const std::string& caller_namespace,
					const std::map<std::string, std::string>& meta,
					const std::vector<struct source_bound>& src_bounds) const
{
	const struct source_bound& src = src_bounds[0]; // make sure there`s only one src

	if ((caller_name != src.service &&
		 caller_name != "*" && src.service != "*") ||
		(caller_namespace != src.service_namespace &&
		 caller_namespace != "*" && src.service_namespace != "*"))
	{
		return false;
	}

	for (const auto &m : meta)
	{
		const auto label_it = src.metadata.find(m.first);
		if (label_it == src.metadata.end() ||
			(label_it->second.value != "*" &&
			!meta_lable_equal(label_it->second, m.second)))
		{
			return false;
		}
	}

	return true;
}

// get_one will be replaced with nearby or others
EndpointAddress *PolarisPolicy::get_one(
		const std::vector<EndpointAddress *>& instances,
		WFNSTracing *tracing)
{
	int x, s = 0;
	int total_weight = 0;	
	size_t i;
	PolarisInstanceParams *params;

	for (i = 0; i < instances.size(); i++)
	{
		if (instances[i]->fail_count < instances[i]->params->max_fails)
		{
			params = static_cast<PolarisInstanceParams *>(instances[i]->params);
			total_weight += params->get_weight();
		}
	}

	if (total_weight == 0) // no healthy servers in the top priority subset
		return instances[rand() % instances.size()];

	if (total_weight > 0)
		x = rand() % total_weight;

	for (i = 0; i < instances.size(); i++)
	{
		if (this->check_server_health(instances[i]) == false)
			continue;

		params = static_cast<PolarisInstanceParams *>(instances[i]->params);
		s += params->get_weight();
		if (s > x)
			break;
	}

	if (i == instances.size())
		i--;

	return instances[i];
}

/*
 * fragment format: #k1=v1&k2=v2&caller_namespace.caller_name
 *
 * if kv pair is for meta router, add "meta" as prefix of each key:
 * 					#meta.k1=v1&meta.k2=v2&caller_namespace.caller_name
 */
bool PolarisPolicy::split_fragment(const char *fragment,
								   std::string& caller_name,
								   std::string& caller_namespace,
								   std::map<std::string, std::string>& meta)
{
	if (fragment == NULL)
		return false;

	std::string caller_info = fragment;
	std::vector<std::string> arr = StringUtil::split(caller_info, '&');
	std::size_t pos;

	if (!arr.empty())
	{
		for (const auto& ele : arr)
		{
			if (ele.empty())
				continue;

			std::vector<std::string> kv = StringUtil::split(ele, '=');

			if (kv.size() == 1)
			{
				caller_info = ele;
				continue;
			}

			if (kv[0].empty() || kv[1].empty())
				return false;

			if (meta.count(kv[0]) > 0)
				continue;

			// If rule_base_router enable, key "meta.xxx" means "meta.xxx".
			// If dst_meta_router enable, key "meta.xxx" means "xxx".
			if (this->config.enable_dst_meta_router)
			{
				pos = kv[0].find("meta.");
				if (pos == std::string::npos)
					continue;
				else
					meta.emplace(kv[0].substr(pos + 5), std::move(kv[1]));
			}
			else
				meta.emplace(std::move(kv[0]), std::move(kv[1]));
		}
	}

	if (this->config.enable_rule_base_router)
	{
		pos = caller_info.find(".");
		if (pos == std::string::npos)
			return false;

		caller_namespace = caller_info.substr(0, pos);
		caller_name = caller_info.substr(pos + 1);
	}

	return true;
}

bool PolarisPolicy::check_server_health(const EndpointAddress *addr)
{
//i	PolarisInstanceParams *params = static_cast<PolarisInstanceParams *>(addr->params);

//	instance->healthy should have a default value.
//	if (params->get_healthy() == false || addr->fail_count > params->max_fails)
	if (addr->fail_count > addr->params->max_fails)
		return false;

	return true;
}

/*
 * Match instance by meta.
 * 1. if some instances are healthy, return them;
 * 2. else if all instances are unheathy, return them, too;
 * 3. else use failover strategy.
 */
bool PolarisPolicy::matching_meta(const std::map<std::string, std::string>& meta,
								  std::vector<EndpointAddress *>& subset)
{
	PolarisInstanceParams *params;
	bool flag;
	std::vector<EndpointAddress *> unhealthy;

	for (size_t i = 0; i < this->servers.size(); i++)
	{
		params = static_cast<PolarisInstanceParams *>(this->servers[i]->params);
		const std::map<std::string, std::string>& inst_meta = params->get_meta();
		flag = true;

		for (const auto &kv : meta)
		{
			const auto inst_meta_it = inst_meta.find(kv.first);

			if (inst_meta_it == inst_meta.end() ||
				kv.second != inst_meta_it->second)
			{
				flag = false;
				break;
			}
		}

		if (flag == true)
		{
			++this->servers[i]->ref;

			if (this->check_server_health(this->servers[i]))
				subset.push_back(this->servers[i]);
			else
				unhealthy.push_back(this->servers[i]);
		}
	}

	if (subset.size())
		return true;

	if (unhealthy.size())
	{
		subset.swap(unhealthy);
		return true;
	}

	switch (this->config.failover_type)
	{
	case MetadataFailoverAll:
		subset = this->servers;
		return true;
	case MetadataFailoverNotKey:
		return this->matching_meta_notkey(meta, subset);
	default:
		return false;
	}
}

// find instances which don`t contain any keys in meta
bool PolarisPolicy::matching_meta_notkey(const std::map<std::string, std::string>& meta,
										 std::vector<EndpointAddress *>& subset)
{
	PolarisInstanceParams *params;
	bool flag;
	std::vector<EndpointAddress *> unhealthy;

	for (size_t i = 0; i < this->servers.size(); i++)
	{
		params = static_cast<PolarisInstanceParams *>(this->servers[i]->params);
		const std::map<std::string, std::string>& inst_meta = params->get_meta();
		flag = true;

		for (const auto &kv : meta)
		{
			if (inst_meta.find(kv.first) != inst_meta.end())
			{
				flag = false;
				break;
			}
		}

		if (flag == true)
		{
			++this->servers[i]->ref;
			if (this->check_server_health(this->servers[i]))
				subset.push_back(this->servers[i]);
			else
				unhealthy.push_back(this->servers[i]);
		}
	}

	if (subset.size())
		return true;

	if (unhealthy.size())
	{
		subset.swap(unhealthy);
		return true;
	}

	return false;
}

}; // namespace polaris


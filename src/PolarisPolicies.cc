#include <set>
#include "PolarisPolicies.h"

namespace polaris {

static constexpr char const *SERVICE_NAMESPACE = "namespace";
static constexpr char const *META_LABLE_EXACT = "EXACT";
static constexpr char const *META_LABLE_REGEX = "REGEX";

static inline bool meta_lable_equal(const struct meta_label& meta,
									const std::string& str)
{
	if (meta.type == META_LABLE_EXACT && meta.value == str)
		return true;

	return false; //TODO support REGEX
}

PolarisPolicyConfig::PolarisPolicyConfig()
{
	this->enable_rule_base_router = true;
	this->enable_nearby_based_router = true;
}

PolarisInstanceParams::PolarisInstanceParams(struct instance inst,
											 const AddressParams *params) :
	PolicyAddrParams(params),
	logic_set(std::move(inst.logic_set)),
	service_namespace(std::move(inst.service_namespace)),
	metadata(std::move(inst.metadata))
{
	this->priority = inst.priority;
	this->enable_healthcheck = inst.enable_healthcheck;
	this->healthy = inst.healthy;
	this->isolate = inst.isolate;

	this->weight = inst.weight;
	// params.weight will not affect here
	// this->weight == 0 has special meaning
}

PolarisPolicy::PolarisPolicy(struct PolarisPolicyConfig& config) :
	config(std::move(config)),
	inbound_rwlock(PTHREAD_RWLOCK_INITIALIZER),
	outbound_rwlock(PTHREAD_RWLOCK_INITIALIZER)
{
	this->total_weight = 0;
	this->available_weight = 0;
}

int PolarisPolicy::init()
{
	return 0; //TODO
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
		addr = new EndpointAddress(name,
						new PolarisInstanceParams(std::move(instances[i]),
												  &params));
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
	EndpointAddress *select_addr;
	std::vector<struct destination_bound> *dst_bounds = NULL;
	std::vector<EndpointAddress *> matched_subset;
	bool ret = true;

	const char *caller = uri.fragment;
	std::map<std::string, std::string> meta = URIParser::split_query(uri.query);

	this->check_breaker();

	if (caller || meta.size())
	{
		if (this->matching_bounds(caller, meta, &dst_bounds))
		{
			if (dst_bounds->size())
			{
				pthread_rwlock_rdlock(&this->rwlock);
				ret = this->matching_subset(dst_bounds, matched_subset);
				pthread_rwlock_unlock(&this->rwlock);
			}
		}
		else
			ret = false;
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
 *
 *	bound_rules not exist : return true
 *	bound_rules exist but match nothing: return false
*/
bool PolarisPolicy::matching_bounds(
					const char *caller_service_name,
					const std::map<std::string, std::string>& meta,
					std::vector<struct destination_bound> **dst_bounds)
{
	std::vector<struct destination_bound> *dst = NULL;
	std::string key = caller_service_name;
	bool ret = true;

	pthread_rwlock_t *lock = &this->inbound_rwlock;
	pthread_rwlock_rdlock(lock);

	BoundRulesMap& rules = this->inbound_rules;
	BoundRulesMap::iterator iter = this->inbound_rules.find(key);

	if (iter == this->inbound_rules.end())
	{
		pthread_rwlock_unlock(lock);
		lock = &this->outbound_rwlock;
		pthread_rwlock_rdlock(lock);
		rules = this->outbound_rules;
	}

	iter = rules.find(key);
	if (iter != rules.end())
	{
		for (struct routing_bound& rule : iter->second)
		{
			if (this->matching_rules(meta, rule.source_bounds))
			{
				dst = &rule.destination_bounds;
				break;
			}
		}

		if (dst)
			*dst_bounds = dst;
		else
			ret = false; // some rules exist but cannot matched
	}

	pthread_rwlock_unlock(lock);
	return ret;
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
			{
				found = true;
			}
		}

		if (found == true)
		{
			bounds = &it->second;
			break;
		}
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
						const std::vector<std::vector<struct EndpointAddress *>>& subsets)
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

		flag = true;
		for (const auto &bound_meta : dst_bounds->metadata)
		{
			const auto &inst_meta = params->get_meta();
			const auto inst_meta_it = inst_meta.find(bound_meta.first);

			if (inst_meta_it == inst_meta.end() ||
				!meta_lable_equal(bound_meta.second, inst_meta_it->second))
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
					const std::map<std::string, std::string>& meta,
					const std::vector<struct source_bound>& src_bounds) const
{
	const struct source_bound& src = src_bounds[0];
	
	// namespace is specially put into meta
	const auto str_it = meta.find(SERVICE_NAMESPACE);
	if (str_it != meta.end() && str_it->second != src.service_namespace)
		return false;

	for (const auto &m : meta)
	{
		const auto label_it = src.metadata.find(m.first);
		if (label_it == src.metadata.end() ||
			!meta_lable_equal(label_it->second, m.second))
		{
			return false;
		}
	}

	return true;
}

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
		if (instances[i]->fail_count > instances[i]->params->max_fails)
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

}; // namespace polaris


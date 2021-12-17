#include "PolarisPolicies.h"

namespace polaris {

PolarisPolicyConfig::PolarisPolicyConfig()
{
	this->enable_rule_base_router = true;
	this->enable_nearby_based_router = true;
}

PolarisInstanceParams::PolarisInstanceParams(struct instance inst,
											 const AddressParams *params) :
	PolicyAddrParams(params),
	id(std::move(inst.id)),
	vpc_id(std::move(inst.vpc_id)),
	protocol(std::move(inst.protocol)),
	version(std::move(inst.version)),
	logic_set(std::move(inst.logic_set)),
	mtime(std::move(inst.mtime)),
	revision(std::move(inst.revision))
//	metadata(std::move(inst.metadata))
{
	this->priority = inst.priority;
	this->enable_healthcheck = inst.enable_healthcheck;
	this->healthy = inst.healthy;
	this->isolate = inst.isolate;

	this->weight = inst.weight;
	// params.weight will not affect here
	// this->weight == 0 has special meaning
}

PolarisPolicy::PolarisPolicy(struct PolarisPolicyConfig config) :
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
	for (size_t i = 0; i < inbounds.size(); i++)
	{
		std::string src_name;
		std::string dst_name = inbounds[i].destination_bounds[0].service;

		if (inbounds[i].source_bounds.size())
			src_name = inbounds[i].source_bounds[0].service;

		MapKey key(std::move(src_name), std::move(dst_name));
		this->inbound_rules.emplace(std::move(key), std::move(inbounds[i]));
	}
}

void PolarisPolicy::update_outbounds(const std::vector<struct routing_bound>& outbounds)
{
	for (size_t i = 0; i < outbounds.size(); i++)
	{
		std::string src_name;
		std::string dst_name = outbounds[i].destination_bounds[0].service;

		if (outbounds[i].source_bounds.size())
			src_name = outbounds[i].source_bounds[0].service;

		MapKey key(std::move(src_name), std::move(dst_name));
		this->outbound_rules.emplace(std::move(key), std::move(outbounds[i]));
	}
}

bool PolarisPolicy::select(const ParsedURI& uri, WFNSTracing *tracing,
						   EndpointAddress **addr)
{
	pthread_rwlock_rdlock(&this->rwlock);
	if (this->servers.size() == 0)
	{
		pthread_rwlock_unlock(&this->rwlock);
		return false;
	}

	this->check_breaker();

	EndpointAddress *select_addr;
	std::map<std::string, struct MatchingString> meta = this->get_request_meta(uri);
	std::vector<struct destination_bound *> dst_bounds;
	std::map<int, std::vector<EndpointAddress *>> matched_subsets;

	if (meta.size())
		this->matching_bounds(meta, dst_bounds);

	if (dst_bounds.size())
		this->matching_instances(dst_bounds, matched_subsets);

	*addr = this->get_one(matched_subsets, tracing);
	++(*addr)->ref;
	pthread_rwlock_unlock(&this->rwlock);

	return true;
}

void PolarisPolicy::matching_bounds(
					const std::map<std::string, struct MatchingString>& meta,
					std::vector<struct destination_bound *>& dst_bounds)
{
	pthread_rwlock_t *lock = &this->inbound_rwlock;
	pthread_rwlock_rdlock(lock);

	std::unordered_map<MapKey, struct routing_bound, KeyHasher> &rules = this->inbound_rules;

	if (this->inbound_rules.size() == 0)
	{
		pthread_rwlock_unlock(lock);
		lock = &this->outbound_rwlock;
		pthread_rwlock_rdlock(lock);
		rules = this->outbound_rules;
	}

	for (auto& rule : rules)
	{
		if (this->matching_rules(meta, rule.second.source_bounds) == true)
		{
			for (size_t j = 0; j < rule.second.destination_bounds.size(); j++)
				dst_bounds.push_back(&(rule.second.destination_bounds[j]));
		}
	}

	pthread_rwlock_unlock(lock);

	return;
}

void PolarisPolicy::matching_instances(
			const std::vector<struct destination_bound *>& dest_bounds,
			std::map<int, std::vector<EndpointAddress *>>& matched_subsets)
{
/*
	size_t i;

	for (auto server : this->servers)
	{
		for (i = 0; i < dst_bound)
		{
			if (server.find(meta) == false)
				break;
		}

		if (this->matching_instance(dst_bounds, server) == true)
		{
			top_priority = server.priority;
			matched_servers.push_back(server);
		}
	}

*/
}

bool PolarisPolicy::matching_rules(
					const std::map<std::string, struct MatchingString>& meta,
					const std::vector<struct source_bound>& src_bounds) const
{
	return true;
}


EndpointAddress *PolarisPolicy::get_one(
		const std::map<int, std::vector<EndpointAddress *>>& matched_subsets,
		WFNSTracing *tracing)
{
/*
	// refer to EndpointGroup::get_one()

	if (this->nalives == 0 || this->matched_subsets.size() == 0)
		// get a random one from this->servers
	else
		// get addr.max_failed < this->max_failed
*/
}

std::map<std::string, struct MatchingString>
PolarisPolicy::get_request_meta(const ParsedURI& uri)
{
	std::map<std::string, struct MatchingString> meta;
	// fill each uri.params into meta
	return meta;
}

}; // namespace polaris


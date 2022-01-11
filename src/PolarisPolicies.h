#ifndef _POLARISPOLICIES_H_
#define _POLARISPOLICIES_H_

#include <utility>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "workflow/URIParser.h"
#include "workflow/EndpointParams.h"
#include "workflow/WFNameService.h"
#include "workflow/WFServiceGovernance.h"
#include "src/PolarisConfig.h"

namespace polaris {

typedef enum MetadataFailoverType {
	MetadataFailoverNone, // default
	MetadataFailoverAll,  // return all instances
	MetadataFailoverNotKey,
} MetadataFailoverType;

/*
struct MatchingString
{
	enum MatchingStringType
	{
		EXACT,
		REGEX,
	};

	MatchingStringType matching_type;

	enum ValueType
	{
		TEXT = 0;
		PARAMETER = 1;
		VARIABLE = 2;
	}

	ValueType value_type;
	std::string value;
};
*/

class PolarisPolicyConfig
{
private:
	std::string service_name;
	std::string location_zone;
	std::string location_region;
	std::string location_campus;
	bool enable_rule_base_router;
	bool enable_dst_meta_router;
	bool enable_nearby_based_router;
	MetadataFailoverType failover_type;

public:
	PolarisPolicyConfig(const std::string& service_name);

	const std::string& get_service_name() const
	{
		return this->service_name;
	}

	void set_rule_base_router_enable()
	{
		this->enable_rule_base_router = true;
		this->enable_dst_meta_router = false;
	}

	void set_dst_meta_router_enable()
	{
		this->enable_dst_meta_router = true;
		this->enable_rule_base_router = false;
	}

	void set_rule_base_router_disable()
	{
		this->enable_rule_base_router = false;
	}

	void set_dst_meta_router_disable()
	{
		this->enable_dst_meta_router = false;
	}

	void set_nearby_based_router_enable()
	{
		this->enable_nearby_based_router = true;
	}

	void set_nearby_based_router_disable()
	{
		this->enable_nearby_based_router = false;
	}

	void set_failover_type(MetadataFailoverType type)
	{
		this->failover_type = type;
	}

	friend class PolarisPolicy;
};

class PolarisInstanceParams : public PolicyAddrParams
{
public:
	int get_weight() const { return this->weight; }
	const std::string& get_namespace() const { return this->service_namespace; }
	const std::map<std::string, std::string>& get_meta() const
	{
		return this->metadata;
	}
	bool get_healthy() const { return this->healthy; }

public:
	PolarisInstanceParams(const struct instance *inst,
						  const struct AddressParams *params);

private:
	int priority;
	int weight;
	bool enable_healthcheck;
	bool healthy;
	bool isolate;
	std::string logic_set;
	std::string service_namespace;
	std::map<std::string, std::string> metadata;
};

class PolarisPolicy : public WFServiceGovernance
{
public:
	PolarisPolicy(const PolarisPolicyConfig *config);

	virtual bool select(const ParsedURI& uri, WFNSTracing *tracing,
						EndpointAddress **addr);

	void update_instances(const std::vector<struct instance>& instances);
	void update_inbounds(const std::vector<struct routing_bound>& inbounds);
	void update_outbounds(const std::vector<struct routing_bound>& outbounds);

private:
	using BoundRulesMap = std::unordered_map<std::string,
											 std::vector<struct routing_bound>>;

	PolarisPolicyConfig config;
	BoundRulesMap inbound_rules;
	BoundRulesMap outbound_rules;
	pthread_rwlock_t inbound_rwlock;
	pthread_rwlock_t outbound_rwlock;

	int total_weight;
	int available_weight;

private:
	virtual void recover_one_server(const EndpointAddress *addr);
	virtual void fuse_one_server(const EndpointAddress *addr);
	virtual void add_server_locked(EndpointAddress *addr);
	void clear_instances_locked();

	void matching_bounds(const std::string& caller_name,
						 const std::string& caller_namespace,
						 const std::map<std::string, std::string>& meta,
						 std::vector<struct destination_bound> **dst_bounds);

	bool matching_subset(
			std::vector<struct destination_bound> *dest_bounds,
			std::vector<EndpointAddress *>& matched_subset);

	bool matching_rules(
			const std::string& caller_name,
			const std::string& caller_namespace,
			const std::map<std::string, std::string>& meta,
			const std::vector<struct source_bound>& src_bounds) const;

	bool matching_instances(struct destination_bound *dst_bounds,
							std::vector<EndpointAddress *>& subsets);

	bool matching_meta(const std::map<std::string, std::string>& meta,
					   std::vector<EndpointAddress *>& subset);
	bool matching_meta_notkey(const std::map<std::string, std::string>& meta,
							  std::vector<EndpointAddress *>& subset);

	size_t subsets_weighted_random(
			const std::vector<struct destination_bound *>& bounds,
			const std::vector<std::vector<EndpointAddress *>>& subsets);

	EndpointAddress *get_one(const std::vector<EndpointAddress *>& instances,
							 WFNSTracing *tracing);

	bool split_fragment(const char *fragment,
						std::string& caller_name,
						std::string& caller_namespace,
						std::map<std::string, std::string>& meta);

	bool check_server_health(const EndpointAddress *addr);
};

}; // namespace polaris

#endif


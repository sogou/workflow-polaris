#include <stdlib.h>
#include <string.h>
#include <gtest/gtest.h>

#include "PolarisPolicies.h"

#include "workflow/UpstreamManager.h"
#include "workflow/WFHttpServer.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"

using namespace polaris;

PolarisConfig config;
static PolarisPolicyConfig conf("b", config);

void init()
{
}

void deinit()
{
}

void fill_inbounds_a_b(std::vector<struct routing_bound>& bounds)
{
/*
	"outbounds": [
		{ "source": [
			{ "service": "a",
			  "namespace": "a_namespace",
			  "metadata": {"k1_env" : "v1_base", "k2_number" : "v2_prime"}
			} ],
		  "destination": [
		  	{ "service": "b",
			  "namespace": "b_namespace",
			  "metadata": {"k1_for_inst_env": "v1_for_inst_base"}, "priority": 1 },
			{ "service": "b",
			  "namespace": "b_namespace",
			  "metadata": {"k1_for_inst_env": "v1_for_inst_grey"}, "priority": 1 },
			{ "service": "b",
			  "namespace": "b_namespace",
			  "metadata": {"k1": "v1"}, {{"k2": "v2"}}, "priority": 2}]
		}
	]
*/
	struct meta_label label_1, label_2;
	label_1.type = "EXACT";
	label_1.value = "v1_base";
	label_2.type = "EXACT";
	label_2.value = "v2_prime";

	std::map<std::string, struct meta_label> meta;
	meta["k1_env"] = label_1;
	meta["k2_number"] = label_2;

	struct source_bound src_a_b_1;
	src_a_b_1.service = "a";
	src_a_b_1.service_namespace = "a_namespace";
	src_a_b_1.metadata = meta;

	struct routing_bound element0;
	element0.source_bounds.push_back(src_a_b_1);

	struct destination_bound dst_a_b_1, dst_a_b_2, dst_a_b_3;
	dst_a_b_1.service = "b";
	dst_a_b_1.priority = 1;
	dst_a_b_1.service_namespace = "b_namespace";
	dst_a_b_2 = dst_a_b_1;
	dst_a_b_3 = dst_a_b_1;

	meta.clear();
	label_1.value = "v1_for_inst_base";
	meta["k1_for_inst_env"] = label_1;
	dst_a_b_1.metadata = meta;
	element0.destination_bounds.push_back(dst_a_b_1);

	meta.clear();
	label_2.value = "v1_for_inst_grey";
	meta["k1_for_inst_env"] = label_2;
	dst_a_b_2.metadata = meta;
	element0.destination_bounds.push_back(dst_a_b_2);
	
	meta.clear();
	label_1.value = "v1";
	label_2.value = "v2";
	meta["k1"] = label_1;
	meta["k2"] = label_2;
	dst_a_b_3.metadata = meta;
	element0.destination_bounds.push_back(dst_a_b_3);

	bounds.push_back(element0);
}

void fill_outbounds_a_b(std::vector<struct routing_bound> &bounds)
{
/*
	"inbounds": [
		{ "source": [
			{ "service": "a",
			  "namespace": "a_namespace",
			  "metadata": {"k1_env" : "v1_base", "k2_number" : "v2_composite"}
			} ],
		  "destination": [
		  	{ "service": "b", "metadata": {"k1_for_inst_env": "v1_for_inst_base"}, "weight": 1 },
			{ "service": "b", "metadata": {"k1_for_inst_env": "v1_for_inst_grey"}, "weight": 99 }]
		}
	]
*/
	struct meta_label label_1, label_2;
	label_1.type = "EXACT";
	label_1.value = "v1_base";
	label_2.type = "EXACT";
	label_2.value = "v2_composite";

	std::map<std::string, struct meta_label> meta;
	meta["k1_env"] = label_1;
	meta["k2_number"] = label_2;

	struct source_bound src_a_b_1;
	src_a_b_1.service = "a";
	src_a_b_1.service_namespace = "a_namespace";
	src_a_b_1.metadata = meta;

	struct routing_bound element0;
	element0.source_bounds.push_back(src_a_b_1);

	struct destination_bound dst_a_b_1, dst_a_b_2, dst_a_b_3;
	dst_a_b_1.service = "b";
	dst_a_b_1.service_namespace = "b_namespace";
	dst_a_b_2 = dst_a_b_1;

	meta.clear();
	label_1.value = "v1_for_inst_base";
	meta["k1_for_inst_env"] = label_1;
	dst_a_b_1.metadata = meta;
	dst_a_b_1.weight = 1;
	element0.destination_bounds.push_back(dst_a_b_1);

	meta.clear();
	label_2.value = "v1_for_inst_grey";
	meta["k1_for_inst_env"] = label_2;
	dst_a_b_2.metadata = meta;
	dst_a_b_2.weight = 99;
	element0.destination_bounds.push_back(dst_a_b_2);
	
	bounds.push_back(element0);
}

void fill_inbounds_c_b(std::vector<struct routing_bound>& bounds)
{
/*
	"outbounds": [
		{ "source": [
			{ "service": "c",
			  "metadata": {"k1_cb" : "v1_cb", "k2_cb" : "v2_cb"}
			} ],
		  "destination": [
		  	{ "service": "b",
			  "metadata": {"k1_cb": "v1_cb"}, "priority": 1 },
			{ "service": "b",
			  "metadata": {"k2_cb": "v2_cb"}, "priority": 1 },
			{ "service": "b",
			  "metadata": {"k1_cb": "v1_cb"}, {{"k2_cb": "v2_cb"}},
			}]
		}
	]
*/
	struct meta_label label_1, label_2;
	label_1.type = "EXACT";
	label_1.value = "v1_cb";
	label_2.type = "EXACT";
	label_2.value = "v2_cb";

	std::map<std::string, struct meta_label> meta;
	meta["k1_cb"] = label_1;
	meta["k2_cb"] = label_2;

	struct source_bound src_c_b_1;
	src_c_b_1.service = "c";
	src_c_b_1.metadata = meta;

	struct routing_bound element0;
	element0.source_bounds.push_back(src_c_b_1);

	struct destination_bound dst_c_b_1, dst_c_b_2, dst_c_b_3;
	dst_c_b_1.service = "b";
	dst_c_b_1.priority = 1;
	dst_c_b_2 = dst_c_b_1;
	dst_c_b_3 = dst_c_b_1;

	meta.clear();
	label_1.value = "v1_cb";
	meta["k1_cb"] = label_1;
	dst_c_b_1.metadata = meta;
	element0.destination_bounds.push_back(dst_c_b_1);

	meta.clear();
	label_2.value = "v1_cb";
	meta["k1_cb"] = label_2;
	dst_c_b_2.metadata = meta;
	element0.destination_bounds.push_back(dst_c_b_2);
	
	meta.clear();
	label_1.value = "v1_cb";
	label_2.value = "v2_cb";
	meta["k1_cb"] = label_1;
	meta["k2_cb"] = label_2;
	dst_c_b_3.metadata = meta;
	dst_c_b_3.priority = 0;
	element0.destination_bounds.push_back(dst_c_b_3);

	bounds.push_back(element0);
}

void fill_instances(std::vector<struct instance>& instances)
{
/*
	instances: [
		{id: "instance_0", host: "b", port: 8000, weight: 10, service_namespace: "b_namespace",
		 meta : {"k1_for_inst_env": "v1_for_inst_base"}},
		{id: "instance_1", host: "b", port: 8001, weight: 1, service_namespace: "b_namespace",
		 meta : {"k1_for_inst_env": "v1_for_inst_grey"}, },
		{id: "instance_2", host: "b", port: 8002, weight: 10000, service_namespace: "b_namespace",
		 meta : {"k1_for_inst_env": "v1_for_inst_grey"}, {"k1": "v1"} },
		{id: "instance_3", host: "b", port: 8003, priority: 9,
		 meta : {"k1": "v1"}, {"k2": "v2"} },
	];
*/
	struct instance inst0, inst1, inst2, inst3;
	inst0.host = "b";
	inst0.service_namespace = "b_namespace";
	inst1 = inst0;
	inst2 = inst0;

	inst0.id = "instance_0";
	inst0.port = 8000;
	inst0.weight = 10;
	inst0.metadata["k1_for_inst_env"] = "v1_for_inst_base";
	instances.push_back(inst0);

	inst1.id = "instance_1";
	inst1.port = 8001;
	inst1.weight = 1;
	inst1.metadata["k1_for_inst_env"] = "v1_for_inst_grey";
	instances.push_back(inst1);
	
	inst2.id = "instance_2";
	inst2.port = 8002;
	inst2.weight = 10000;
	inst2.metadata["k1"] = "v1";
	inst2.metadata["k1_for_inst_env"] = "v1_for_inst_grey";
	instances.push_back(inst2);

	inst3.host = "b";
	inst3.id = "instance_3";
	inst3.port = 8003;
	inst3.priority = 9;
	inst3.metadata["k1"] = "v1";
	inst3.metadata["k2"] = "v2";
	instances.push_back(inst3);
}

/*
void check_inbounds_a_b(PolarisPolicy::BoundRulesMap& bound_map)
{
	std::vector<struct routing_bound>& bounds = bound_map["a"];
	EXPECT_EQ(bounds[0].destination_bounds.size(), 3);
	EXPECT_TRUE(bounds[0].source_bounds[0].service == "a");
	EXPECT_TRUE(bounds[0].source_bounds[0].service_namespace == "a_namespace");
	EXPECT_TRUE(bounds[0].destination_bounds[0].service == "b");
	EXPECT_EQ(bounds[0].destination_bounds[0].metadata.size(), 1);
	EXPECT_EQ(bounds[0].destination_bounds[1].metadata.size(), 1);
	EXPECT_EQ(bounds[0].destination_bounds[2].metadata.size(), 2);
	EXPECT_TRUE(bounds[0].destination_bounds[1].metadata["k1_for_inst_env"].value == "v1_for_inst_grey");
	EXPECT_EQ(bounds[0].destination_bounds[2].priority, 1);
}

void check_outbounds_a_b(PolarisPolicy::BoundRulesMap& bound_map)
{
	std::vector<struct routing_bound>& bounds = bound_map["a"];
	EXPECT_EQ(bounds.size(), 1);
	EXPECT_EQ(bounds[0].destination_bounds.size(), 2);
	EXPECT_TRUE(bounds[0].source_bounds[0].service == "a");
	EXPECT_TRUE(bounds[0].source_bounds[0].metadata["k2_number"].value == "v2_composite");
	EXPECT_TRUE(bounds[0].destination_bounds[0].service_namespace == "b_namespace");
	EXPECT_TRUE(bounds[0].destination_bounds[1].metadata["k1_for_inst_env"].value == "v1_for_inst_grey");
	EXPECT_EQ(bounds[0].destination_bounds[1].weight, 99);
}

void check_inbounds_c_b(PolarisPolicy::BoundRulesMap& bound_map)
{
	std::vector<struct routing_bound>& bounds = bound_map["c"];
	EXPECT_EQ(bounds[0].destination_bounds.size(), 3);
	EXPECT_TRUE(bounds[0].source_bounds[0].service == "c");
	EXPECT_TRUE(bounds[0].destination_bounds[0].service == "b");
	EXPECT_TRUE(bounds[0].destination_bounds[1].metadata["k1_cb"].value == "v1_cb");
	EXPECT_TRUE(bounds[0].destination_bounds[2].metadata["k2_cb"].value == "v2_cb");
}

TEST(polaris_policy_unittest, update_bounds)
{
	std::vector<struct routing_bound> routing_inbounds;
	std::vector<struct routing_bound> routing_outbounds;

	fill_inbounds_a_b(routing_inbounds);
	fill_outbounds_a_b(routing_outbounds);

	PolarisPolicy pp(&conf);

	pp.update_inbounds(routing_inbounds);
	pp.update_outbounds(routing_outbounds);
	check_inbounds_a_b(pp.inbound_rules);
	check_outbounds_a_b(pp.outbound_rules);

	pp.update_inbounds(routing_inbounds);
	EXPECT_EQ(pp.inbound_rules.size(), 1);
	EXPECT_EQ(pp.outbound_rules.size(), 1);
	check_inbounds_a_b(pp.inbound_rules);
	check_outbounds_a_b(pp.outbound_rules);
}

TEST(polaris_policy_unittest, update_mulitple_bounds)
{
	std::vector<struct routing_bound> routing_inbounds;

	PolarisPolicy pp(&conf);
	fill_inbounds_a_b(routing_inbounds);
	pp.update_inbounds(routing_inbounds);
	fill_inbounds_c_b(routing_inbounds);
	pp.update_inbounds(routing_inbounds);
	EXPECT_EQ(pp.inbound_rules.size(), 2);
	check_inbounds_a_b(pp.inbound_rules);
	check_inbounds_c_b(pp.inbound_rules);
}

TEST(polaris_policy_unittest, update_instances)
{
	std::vector<struct instance> instances;

	fill_instances(instances);

	PolarisPolicy pp(&conf);
	pp.update_instances(instances);
	pp.update_instances(instances);

	EXPECT_EQ(instances.size(), 4);
	EXPECT_TRUE(instances[0].id == "instance_0");
	EXPECT_TRUE(instances[1].metadata["k1_for_inst_env"] == "v1_for_inst_grey");
	EXPECT_TRUE(instances[2].metadata["k1"] == "v1");
	EXPECT_EQ(instances[3].priority, 9);
}

TEST(polaris_policy_unittest, matching_bounds)
{
	std::vector<struct routing_bound> routing_inbounds;
	fill_inbounds_a_b(routing_inbounds);

	PolarisPolicy pp(&conf);
	pp.update_inbounds(routing_inbounds);
	
	const char *caller = "a";
	std::map<std::string, std::string> meta;
	std::vector<struct destination_bound> *dst_bounds = NULL;

//	1. "metadata": {"k1_env" : "v1_base", "k2_number" : "v2_prime"}
	meta["k1_env"] = "v1_base";
	meta["k2_number"] = "v2_prime";	
	pp.matching_bounds(caller, meta, &dst_bounds);
	EXPECT_EQ(dst_bounds->size(), 3);

//	2. caller : "c"
	dst_bounds = NULL;
	pp.matching_bounds("c", meta, &dst_bounds);
	EXPECT_TRUE(dst_bounds == NULL);

//	3. namespace: "not_a_namespace"
	dst_bounds = NULL;
	meta["namespace"] = "not_a_namespace";
	pp.matching_bounds(caller, meta, &dst_bounds);
	EXPECT_TRUE(dst_bounds == NULL);
}

TEST(polaris_policy_unittest, matching_instances)
{
	std::vector<struct routing_bound> routing_inbounds;
	fill_inbounds_a_b(routing_inbounds);

	std::vector<struct instance> instances;
	fill_instances(instances);

	PolarisPolicy pp(&conf);
	pp.update_instances(instances);
	pp.update_inbounds(routing_inbounds);

	struct destination_bound *dst_bound;
	dst_bound = &(routing_inbounds[0].destination_bounds[1]);

	std::vector<EndpointAddress *> subsets;
	pp.matching_instances(dst_bound, subsets);

	EXPECT_EQ(subsets.size(), 2);

	if (atoi(subsets[0]->port.c_str()) == 8001)
		EXPECT_EQ(atoi(subsets[1]->port.c_str()), 8002);
	else
	{
		EXPECT_EQ(atoi(subsets[0]->port.c_str()), 8002);
		EXPECT_EQ(atoi(subsets[1]->port.c_str()), 8001);
	}
}

TEST(polaris_policy_unittest, matching_subset)
{
	std::vector<struct routing_bound> routing_inbounds;
	fill_inbounds_a_b(routing_inbounds);

	std::vector<struct instance> instances;
	fill_instances(instances);

	PolarisPolicy pp(&conf);
	pp.update_instances(instances);
	pp.update_inbounds(routing_inbounds);

	std::vector<struct destination_bound> *dst_bounds;
	dst_bounds = &routing_inbounds[0].destination_bounds;

	std::vector<EndpointAddress *> subsets;
	pp.matching_subset(dst_bounds, subsets);

	EXPECT_EQ(subsets.size(), 2);
	for (const auto& server : subsets)
	{
		unsigned short port = atoi(server->port.c_str());
		if (port == 8001 || port == 8002)
			EXPECT_TRUE(true);
		else
			EXPECT_TRUE(false);
	}
}

TEST(polaris_policy_unittest, split_fragment)
{
	PolarisPolicy pp(&conf);

	std::string caller_name;
	std::string caller_namespace;
	std::map<std::string, std::string> meta;
	const char *fragment = "k1=v1&k2=v2&caller_namespace.caller_name";

	EXPECT_TRUE(pp.split_fragment(fragment, meta, caller_name, caller_namespace));
	EXPECT_EQ(meta.size(), 2);
	EXPECT_TRUE(caller_name == "caller_name");
	EXPECT_TRUE(caller_namespace == "caller_namespace");

	fragment = "k1=v1&caller_namespace.";
	EXPECT_FALSE(pp.split_fragment(fragment, meta, caller_name, caller_namespace));

	fragment = "k1=v1&.";
	EXPECT_FALSE(pp.split_fragment(fragment, meta, caller_name, caller_namespace));

	fragment = "k1=v1&k2=&caller_namespace.caller_name";
	EXPECT_FALSE(pp.split_fragment(fragment, meta, caller_name, caller_namespace));

	caller_name.clear();
	caller_namespace.clear();
	meta.clear();
	fragment = "*.*";
	EXPECT_TRUE(pp.split_fragment(fragment, meta, caller_name, caller_namespace));
	EXPECT_TRUE(caller_name == "*");
	EXPECT_TRUE(caller_namespace == "*");
}
*/

TEST(polaris_policy_unittest, select)
{	
	std::vector<struct routing_bound> routing_inbounds;
	fill_inbounds_a_b(routing_inbounds);

	std::vector<struct instance> instances;
	fill_instances(instances);

	PolarisPolicy pp(&conf);
	pp.update_instances(instances);
	pp.update_inbounds(routing_inbounds);

	EndpointAddress *addr;
	ParsedURI uri;

	std::string url = "http://b_namespace.b:8080#k1_env=v1_base&k2_number=v2_prime&a_namespace.a";
	EXPECT_EQ(URIParser::parse(url, uri), 0);

	pp.select(uri, NULL, &addr);
	EXPECT_EQ(atoi(addr->port.c_str()), 8002);
}

TEST(polaris_policy_unittest, meta_router)
{
	std::vector<struct routing_bound> routing_inbounds;
	fill_inbounds_a_b(routing_inbounds);

	std::vector<struct instance> instances;
	fill_instances(instances);

	conf.set_dst_meta_router(true);
	PolarisPolicy pp(&conf);
	pp.update_instances(instances);
	pp.update_inbounds(routing_inbounds);

	EndpointAddress *addr;
	ParsedURI uri;
	std::string url = "http://b_namespace.b:8080#meta.k1=v1&meta.k2=v2";
	EXPECT_EQ(URIParser::parse(url, uri), 0);

	pp.select(uri, NULL, &addr);
	EXPECT_EQ(atoi(addr->port.c_str()), 8003);

	url = "http://b_namespace.b:8080#meta.k1=v1";
	EXPECT_EQ(URIParser::parse(url, uri), 0);

	pp.select(uri, NULL, &addr);
	EXPECT_EQ(atoi(addr->port.c_str()), 8002);
	conf.set_rule_base_router(true);
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);

	init();

	EXPECT_EQ(RUN_ALL_TESTS(), 0);

	deinit();

	return 0;
}


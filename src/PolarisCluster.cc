#include "PolarisCluster.h"
#include "PolarisPolicy.h"
#include "workflow/WFGlobal.h"

namespace polaris {

// must call inside this->mutex
void PolarisCluster::reset_healthcheck_cluster(const std::vector<struct instance>& instances)
{
    std::vector<struct instance> filtered_instances;
    for (const auto& inst : instances)
    {
        if (inst.protocol == "http")
            filtered_instances.push_back(inst);
    }
    
    WFNameService *ns = WFGlobal::get_name_service();
    PolarisPolicy *pp;

    pp = dynamic_cast<PolarisPolicy *>(ns->get_policy(this->data->healthcheck_policy.c_str()));

    if (pp == NULL)
    {
        PolarisConfig config;
        PolarisPolicyConfig conf(this->data->healthcheck_policy, config);
        pp = new PolarisPolicy(&conf);
        ns->add_policy(this->data->healthcheck_policy.c_str(), pp);
    }

    pp->update_instances(filtered_instances);
}

void PolarisCluster::incref()
{
    (*this->ref)++;
}

void PolarisCluster::decref()
{
    int ref_count = --*this->ref;

    if (ref_count == 0)
    {
        delete WFGlobal::get_name_service()->del_policy(this->data->healthcheck_policy.c_str());
        delete this->ref;
        delete this->mutex;
        delete this->status;
        delete this->data;
    }
}

} // namespace polaris

#ifndef _POLARISCLUSTER_H_
#define _POLARISCLUSTER_H_

namespace polaris {

class PolarisCluster {
  public:
    PolarisCluster() {
        this->ref = new std::atomic<int>(1);
        this->status = new int(0);
        this->mutex = new std::mutex;
        this->data = new struct cluster_data;
    }

    virtual ~PolarisCluster() { this->decref(); }

    PolarisCluster(const PolarisCluster &copy) {
        this->ref = copy.ref;
        this->mutex = copy.mutex;
        this->status = copy.status;
        this->data = copy.data;
        this->incref();
    }

    PolarisCluster &operator=(const PolarisCluster &copy) {
        if (this != &copy) {
            this->decref();
            this->ref = copy.ref;
            this->mutex = copy.mutex;
            this->status = copy.status;
            this->data = copy.data;
            this->incref();
        }
        return *this;
    }

    std::mutex *get_mutex() { return this->mutex; }
    int *get_status() { return this->status; }
    std::vector<std::string> *get_server_connectors() { return &this->data->server_connectors; }
    std::vector<std::string> *get_discover_clusters() { return &this->data->discover_clusters; }
    std::vector<std::string> *get_healthcheck_clusters() { return &this->data->healthcheck_clusters; }
    std::vector<std::string> *get_monitor_clusters() { return &this->data->monitor_clusters; }
    std::vector<std::string> *get_metrics_clusters() { return &this->data->metrics_clusters; }
    std::map<std::string, std::string> *get_revision_map() { return &this->data->revision_map; }

    int discover_failed() { return ++this->data->discover_failed_cnt; }
    void clear_discover_failed() { this->data->discover_failed_cnt = 0; }
    int healthcheck_failed() { return ++this->data->healthcheck_failed_cnt; }
    void clear_healthcheck_failed() { this->data->healthcheck_failed_cnt = 0; }

  private:
    void incref() { (*this->ref)++; }

    void decref() {
        if (--*this->ref == 0) {
            delete this->ref;
            delete this->mutex;
            delete this->status;
            delete this->data;
        }
    }

  private:
    struct cluster_data
    {
        std::vector<std::string> server_connectors;
        std::vector<std::string> discover_clusters;
        std::vector<std::string> healthcheck_clusters;
        std::vector<std::string> monitor_clusters;
        std::vector<std::string> metrics_clusters;
        std::map<std::string, std::string> revision_map;
        int discover_failed_cnt;
        int healthcheck_failed_cnt;
        //TODO: add other fail count when other cluster requests are supported

        cluster_data()
        {
            discover_failed_cnt = 0;
            healthcheck_failed_cnt = 0;
        }
    };

  private:
    std::atomic<int> *ref;
    int *status;
    std::mutex *mutex;
    struct cluster_data *data;
};

};  // namespace polaris

#endif

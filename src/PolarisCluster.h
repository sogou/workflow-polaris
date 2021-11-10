#ifndef _POLARISCLUSTER_H_
#define _POLARISCLUSTER_H_

namespace polaris {

class PolarisCluster {
  public:
    PolarisCluster() {
        this->ref = new std::atomic<int>(1);
        status = new int(0);
        mutex = new std::mutex;
        this->server_connectors = new std::vector<std::string>;
        this->discover_clusters = new std::vector<std::string>;
        this->healthcheck_clusters = new std::vector<std::string>;
        this->monitor_clusters = new std::vector<std::string>;
        this->metrics_clusters = new std::vector<std::string>;
        this->revision_map = new std::map<std::string, std::string>;
    }

    virtual ~PolarisCluster() { this->decref(); }

    PolarisCluster(const PolarisCluster &copy) {
        this->ref = copy.ref;
        this->mutex = copy.mutex;
        this->status = copy.status;
        this->server_connectors = copy.server_connectors;
        this->discover_clusters = copy.discover_clusters;
        this->healthcheck_clusters = copy.healthcheck_clusters;
        this->monitor_clusters = copy.monitor_clusters;
        this->metrics_clusters = copy.metrics_clusters;
        this->revision_map = copy.revision_map;
        this->incref();
    }

    PolarisCluster &operator=(const PolarisCluster &copy) {
        if (this != &copy) {
            this->decref();
            this->ref = copy.ref;
            this->mutex = copy.mutex;
            this->status = copy.status;
            this->server_connectors = copy.server_connectors;
            this->discover_clusters = copy.discover_clusters;
            this->healthcheck_clusters = copy.healthcheck_clusters;
            this->monitor_clusters = copy.monitor_clusters;
            this->metrics_clusters = copy.metrics_clusters;
            this->revision_map = copy.revision_map;
            this->incref();
        }
        return *this;
    }

    std::mutex *get_mutex() { return this->mutex; }
    int *get_status() { return this->status; }
    std::vector<std::string> *get_server_connectors() { return this->server_connectors; }
    std::vector<std::string> *get_discover_clusters() { return this->discover_clusters; }
    std::map<std::string, std::string> *get_revision_map() { return this->revision_map; }

  private:
    void incref() { (*this->ref)++; }

    void decref() {
        if (--*this->ref == 0) {
            delete this->ref;
            delete this->mutex;
            delete this->status;
            delete this->server_connectors;
            delete this->discover_clusters;
            delete this->healthcheck_clusters;
            delete this->monitor_clusters;
            delete this->metrics_clusters;
            delete this->revision_map;
        }
    }

  private:
    std::atomic<int> *ref;
    int *status;
    std::mutex *mutex;
    std::vector<std::string> *server_connectors;
    std::vector<std::string> *discover_clusters;
    std::vector<std::string> *healthcheck_clusters;
    std::vector<std::string> *monitor_clusters;
    std::vector<std::string> *metrics_clusters;
    std::map<std::string, std::string> *revision_map;
};

};  // namespace polaris

#endif

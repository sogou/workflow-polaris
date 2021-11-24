#ifndef _POLARISCLUSTER_H_
#define _POLARISCLUSTER_H_

namespace polaris {

struct inner_cluster_t {
    std::vector<std::string> server_connectors;
    std::vector<std::string> discover_clusters;
    std::vector<std::string> healthcheck_clusters;
    std::vector<std::string> monitor_clusters;
    std::vector<std::string> metrics_clusters;
    std::map<std::string, std::string> revision_map;
};

class PolarisCluster {
  public:
    PolarisCluster() {
        this->ref = new std::atomic<int>(1);
        this->status = new int(0);
        this->mutex = new std::mutex;
        this->inner_cluster = new inner_cluster_t;
    }

    virtual ~PolarisCluster() { this->decref(); }

    PolarisCluster(const PolarisCluster &copy) {
        this->ref = copy.ref;
        this->mutex = copy.mutex;
        this->status = copy.status;
        this->inner_cluster = copy.inner_cluster;
        this->incref();
    }

    PolarisCluster &operator=(const PolarisCluster &copy) {
        if (this != &copy) {
            this->decref();
            this->ref = copy.ref;
            this->mutex = copy.mutex;
            this->status = copy.status;
            this->inner_cluster = copy.inner_cluster;
            this->incref();
        }
        return *this;
    }

    std::mutex *get_mutex() { return this->mutex; }
    int *get_status() { return this->status; }
    struct inner_cluster_t *get_inner_cluster() {
        return this->inner_cluster;
    }

  private:
    void incref() { (*this->ref)++; }

    void decref() {
        if (--*this->ref == 0) {
            delete this->ref;
            delete this->mutex;
            delete this->status;
            delete this->inner_cluster;
        }
    }

  private:
    std::atomic<int> *ref;
    int *status;
    std::mutex *mutex;
    struct inner_cluster_t *inner_cluster;
};

};  // namespace polaris

#endif

#include <string>
#include <vector>

#include <sysrepo-cpp/Sysrepo.hpp>

using namespace std;

namespace pdns_conf
{
void writeConfig(const string& fpath, const vector<sysrepo::S_Val>& values);
} // namespace pdns_conf
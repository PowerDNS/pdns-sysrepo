#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>

#include <sysrepo-cpp/Sysrepo.hpp>
#include <sysrepo-cpp/Session.hpp>
#include <boost/filesystem.hpp>
#include <mstch/mstch.hpp>

using namespace std;
namespace fs = boost::filesystem;

namespace pdns_conf {
    static const string pdns_conf_template = R"(
local-address =
{{ #local-address }}
local-address += {{ address }}
{{ /local-address }}
master = {{ master }}
slave = {{ slave }}
)";

    void writeConfig(const string &fpath, const vector<sysrepo::S_Val> &values) {
        auto p = fs::path(fpath);
        auto d = p.remove_filename();
        if (!fs::is_directory(d)) {
            // TODO find better exception
            throw range_error(d.string() + " is not a directory");
        }
        mstch::map ctx{
            {"master", string("no")},
            {"slave", string("no")},
            {"local-address", mstch::array{
                mstch::map{{"address", string("127.0.0.1:53")}}
                }
            }
        };
        for (auto const &v: values) {
            if (string(v->xpath()) == "/pdns-server/pdns-server/master") {
                ctx["master"] = v->val_to_string();
            }
            if (string(v->xpath()) == "/pdns-server/pdns-server/slave") {
                ctx["slave"] = v->val_to_string();
            }
            // TODO parse out the list of listen addrs.
        }

        // TODO tmpfile
        std::ofstream outputFile(fpath);
        if (!outputFile) {
            throw runtime_error("Unable to open output file '"+ fpath + "'");
        }

        outputFile<<mstch::render(pdns_conf_template, ctx);
        outputFile.close();
    }
}
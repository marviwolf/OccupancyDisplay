#include "json/valijson_nlohmann.hpp"
#include <string>

void initTmp();
void exitTmp();
bool schemaValidate();
void pairNode(std::string nodeid, std::string pincode, std::string discriminator, bool retry, std::string datasetkey);
void unpairNode(std::string nodeid, bool retry, std::string datasetkey);
void updateNodes();
int main(int argc, char * argv[]);
void switchDummy(int count);
#include "controller.h"
#include <filesystem>
#include <fstream>
#include <unistd.h>

// cluster, in which "change-channel" is contained - set by ZAP tool in display-common/display-app.zap
#define CHANGE_CHANNEL_CLUSTER "1"
// chip config directory in the filesystem used by "chip-tool-release"
#define CHIP_CONFIG_DIRECTORY "/tmp/"
// saveplace for chip config files -> persistence after reboot
#define CHIP_CONFIG_SAVE_DIRECTORY "chip-config/"
// define how often all displays should update their information
#define UPDATE_INTERVAL_MINUTES 15

#define concat(first, second) first second

namespace file = std::filesystem;
using std::string;
using json = nlohmann::json;

static const file::copy_options cpOptions = file::copy_options::overwrite_existing;
static const char * weekdays[]            = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// copies persistent files to chip-tool config directory
void initTmp()
{
    file::copy(concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_config.ini"), concat(CHIP_CONFIG_DIRECTORY, "chip_config.ini"), cpOptions);
    file::copy(concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_counters.ini"), concat(CHIP_CONFIG_DIRECTORY, "chip_counters.ini"),
               cpOptions);
    file::copy(concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_kvs"), concat(CHIP_CONFIG_DIRECTORY, "chip_kvs"), cpOptions);
    file::copy(concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_tool_config.alpha.ini"),
               concat(CHIP_CONFIG_DIRECTORY, "chip_tool_config.alpha.ini"), cpOptions);
    file::copy(concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_tool_config.ini"), concat(CHIP_CONFIG_DIRECTORY, "chip_tool_config.ini"),
               cpOptions);
    file::copy(concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_tool_history"), concat(CHIP_CONFIG_DIRECTORY, "chip_tool_history"),
               cpOptions);

    printf("Chip Tool config files copied from %s to %s\n", CHIP_CONFIG_SAVE_DIRECTORY, CHIP_CONFIG_DIRECTORY);
}

// saves files from chip-tool config directory to persistent location
void exitTmp()
{
    file::copy(concat(CHIP_CONFIG_DIRECTORY, "chip_config.ini"), concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_config.ini"), cpOptions);
    file::copy(concat(CHIP_CONFIG_DIRECTORY, "chip_counters.ini"), concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_counters.ini"),
               cpOptions);
    file::copy(concat(CHIP_CONFIG_DIRECTORY, "chip_kvs"), concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_kvs"), cpOptions);
    file::copy(concat(CHIP_CONFIG_DIRECTORY, "chip_tool_config.alpha.ini"),
               concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_tool_config.alpha.ini"), cpOptions);
    file::copy(concat(CHIP_CONFIG_DIRECTORY, "chip_tool_config.ini"), concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_tool_config.ini"),
               cpOptions);
    file::copy(concat(CHIP_CONFIG_DIRECTORY, "chip_tool_history"), concat(CHIP_CONFIG_SAVE_DIRECTORY, "chip_tool_history"),
               cpOptions);

    printf("Chip Tool config files copied from %s to %s\n", CHIP_CONFIG_DIRECTORY, CHIP_CONFIG_SAVE_DIRECTORY);
}

/**
 * - validates user-input data given by "prod-data.json"
 * - assures, data isn't missing and data types are correct
 * - prevents problems with the screens and server-software
 * - returns true if "prod-data.json" is valid, else false
 */
bool schemaValidate()
{
    // schema file is loaded and initialized
    nlohmann::basic_json schemaDoc;
    if (!valijson::utils::loadDocument("schema.json", schemaDoc))
    {
        printf("Failed to load schema document\n");
        return false;
    }
    valijson::Schema schema;
    valijson::SchemaParser parser;
    valijson::adapters::NlohmannJsonAdapter schemaAdapter(schemaDoc);
    parser.populateSchema(schemaAdapter, schema);

    // json file to check is loaded
    nlohmann::basic_json targetDoc;
    if (!valijson::utils::loadDocument("prod-data.json", targetDoc))
    {
        printf("Failed to load target document\n");
        return false;
    }

    // validity of json using schema is checked
    valijson::Validator validator;
    valijson::adapters::NlohmannJsonAdapter targetAdapter(targetDoc);
    if (!validator.validate(schema, targetAdapter, NULL))
    {
        printf("Data schema validation failed\n");
        return false;
    }
    else
    {
        printf("Data schema validated!\n");
        return true;
    }
}

/**
 * - pairs / commissions a new device node into the Matter (and Thread) network
 * - uses strings of device nodeid, pincode and discriminator to pair specific device
 * - uses Thread datasetkey to get into the network
 * - boolean retry indicates, if pairing should be retried in case of error
 */
void pairNode(string nodeid, string pincode, string discriminator, bool retry, string datasetkey)
{
    // if nodeid object in "devices.json" already exists, dont try pairing
    json devices = json::parse(std::ifstream("devices.json"));
    /*  if (devices[nodeid].is_object())
     {
         return;
     } */

    printf("Pairing\n");
    // command is created with output into tmp file
    char command[1000];
    snprintf(command, 999, "./chip-tool-release pairing ble-thread %s hex:%s %s %s >> tmp.out", nodeid.c_str(), datasetkey.c_str(),
             pincode.c_str(), discriminator.c_str());

    // remove tmp.out if existing somehow
    std::remove("tmp.out");

    // command is running
    system(command);

    // result of command is evaluated
    std::ifstream tmp_out("tmp.out");
    if (tmp_out.is_open())
    {
        string tmp = "";
        bool found = false;
        // read every line of the tmp output file
        while (std::getline(tmp_out, tmp))
        {
            // commissionng worked
            if (tmp.find("Device commissioning completed with success") != -1)
            {
                found = true;
                // exit while-loop, no need for checking more lines
                break;
            }
        }

        if (found)
        {
            printf("Successfully paired device with node %s\n", nodeid.c_str());

            // create entry in "devices.json"
            json arr;
            arr["discriminator"] = discriminator;
            arr["pincode"]       = pincode;
            json obj;
            obj[nodeid]  = arr;
            json devices = json::parse(std::ifstream("devices.json"));
            devices.update(obj);
            std::ofstream deviceFile("devices.json");
            deviceFile << devices;
            deviceFile.close();

            // tmp file is deleted
            tmp_out.close();
            std::remove("tmp.out");

            printf("Created node for id %s in devices.json: %s\n", nodeid.c_str(), devices[nodeid].dump().c_str());
        }
        else
        {
            /**
             * - simple troubleshooting, if device or chip-tool bug a little bit
             * - node is unpaired first, then paired again
             * - actually solves some problems
             * */
            if (retry)
            {
                printf("Pairing of node %s failed!\nTrying unpair + pair again\n", nodeid.c_str());
                unpairNode(nodeid, false, datasetkey);
                pairNode(nodeid, pincode, discriminator, false, datasetkey);
            }
            // commmissioning failed
            else
            {
                printf("Pairing of node %s failed!\n", nodeid.c_str());
            }
        }
    }
}

/**
 * - unpairs / uncommissions an existing device
 * - uses strings of device nodeid to unpair specific device
 * - uses Thread datasetkey only in case of failure
 * - boolean retry indicates, if unpairing should be retried in case of error
 */
void unpairNode(string nodeid, bool retry, string datasetkey)
{
    // if nodeid isn't in "devices.json", dont try unpairing
    json devices = json::parse(std::ifstream("devices.json"));
    /*     if (devices[nodeid].is_null())
        {
            return;
        } */

    printf("Unpairing\n");
    // command is created with output into tmp file
    char command[1000];
    snprintf(command, 999, "./chip-tool-release pairing unpair %s >> tmp.out", nodeid.c_str());

    // remove tmp.out if existing somehow
    std::remove("tmp.out");

    // command is running
    system(command);

    // result of command is evaluated
    std::ifstream tmp_out("tmp.out");
    if (tmp_out.is_open())
    {
        string tmp = "";
        bool found = false;
        // read every line of the tmp output file
        while (std::getline(tmp_out, tmp))
        {
            // uncommissioning worked
            if (tmp.find("Device unpair completed with success") != -1)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            printf("Successfully unpaired device with node %s\n", nodeid.c_str());

            // deleting entry in "devices.json"
            json devices = json::parse(std::ifstream("devices.json"));
            if (!devices[nodeid].is_null())
            {
                printf("Found node for nodeid %s in devices.json and deleted it: %s\n", nodeid.c_str(),
                       devices[nodeid].dump().c_str());
            }
            devices.erase(nodeid);
            std::ofstream deviceFile("devices.json");
            deviceFile << devices;
            deviceFile.close();
            // deleting tmp file
            tmp_out.close();
            std::remove("tmp.out");
        }
        else
        {
            /**
             * - simple troubleshooting, if device or chip-tool bug a little bit
             * - node is paired first, then unpaired again
             * - actually solves some problems
             * */
            if (retry)
            {
                printf("Unpairing of node %s failed!\nTrying pair + unpair again\n", nodeid.c_str());
                json devices = json::parse(std::ifstream("devices.json"));
                pairNode(nodeid, devices[nodeid]["pincode"].get<string>(), devices[nodeid]["discriminator"].get<string>(), false,
                         datasetkey);
                unpairNode(nodeid, false, datasetkey);
            }
            // uncommissioning failed
            else
            {
                printf("Unpairing of node %s failed!\n", nodeid.c_str());
            }
        }
    }
}
/**
 * - every node listed in "prod-data.json" gets updated
 * - node also has to be paired, so existing in "devices.json"
 * -
 */
void updateNodes()
{
    // reading json files every cycle
    json rooms   = json::parse(std::ifstream("prod-data.json"));
    json devices = json::parse(std::ifstream("devices.json"));

    // get current date and time
    time_t now  = time(0);
    tm * ltm    = localtime(&now);
    string date = std::to_string(ltm->tm_mday) + "." + std::to_string(1 + ltm->tm_mon) + "." + std::to_string(1900 + ltm->tm_year);
    string time = std::to_string(ltm->tm_hour) + ":" + std::to_string(ltm->tm_min);
    string weekday = weekdays[ltm->tm_wday];

    // iterating through every room entry in "prod-data.json"
    for (auto room : rooms["room"])
    {
        string nodeid = to_string(room["nodeid"]);
        // command string start
        string command = "./chip-tool-release channel change-channel '`";

        printf("Updating node %s\n", nodeid.c_str());

        // device nodeid has to exist in "devices.json" and discrimator / pincode have to match
        if (devices[nodeid].is_object() && to_string(room["discriminator"]) == devices[nodeid]["discriminator"].get<string>() &&
            to_string(room["pincode"]) == devices[nodeid]["pincode"].get<string>())
        {
            // add nodeid, roomnumber, weekday, date and time to the command
            command += nodeid + "`" + room["roomnumber"].get<string>() + "`" + weekday + "`" + date + "`" + time + "`";

            // every entry of the room is added to the command
            for (auto entry : room["entry"])
            {
                command += entry["lecturename"].get<string>() + "`" + entry["subtitle"].get<string>() + "`" +
                    to_string(entry["starthour"]) + "`" + to_string(entry["startminute"]) + "`" + to_string(entry["endhour"]) +
                    "`" + to_string(entry["endminute"]) + "`";
            }

            // end of the command
            command += "' " + nodeid + " " + string(CHANGE_CHANNEL_CLUSTER) + " >> tmp.out";

            // remove tmp.out if existing somehow
            std::remove("tmp.out");

            // command is executed
            system(command.c_str());
            std::ifstream tmp_out("tmp.out");
            if (tmp_out.is_open())
            {
                string tmp = "";
                while (std::getline(tmp_out, tmp))
                {
                    /**
                     * - search one of the possible responses defined in ChannelManager.cpp
                     *      -> void ChannelManager::HandleChangeChannel(...)
                     * - if found, that means the Matter command was successfully recieved and responded to
                     */
                    if (tmp.find("data: Successful update of screen") != -1 || tmp.find("data: Information wasnt new") != -1)
                    {
                        printf("Successful update of node %s\n", nodeid.c_str());
                        // exit while-loop, no need for checking more lines
                        break;
                    }
                }
            }
            // tmp file is deleted
            tmp_out.close();
            std::remove("tmp.out");
        }
        // device isn't in "devices.json"
        else
        {
            printf("Node with id %s isn't paired or discriminator / pin code dont match!\n-> Skipping update of room %s!\n",
                   nodeid.c_str(), room["roomnumber"].get<string>().c_str());
        }
    }
}

/**
 * - changes the content of "prod-data.json" based on the integer input
 * - only for testing / demonstration purposes, not for production
 * - switches between the 5 dummy.json file contents
 */
void switchDummy(int count)
{
    if (count % 4 == 0)
    {
        file::copy("dummy1.json", "prod-data.json", cpOptions);
    }
    if (count % 4 == 1)
    {
        file::copy("dummy2.json", "prod-data.json", cpOptions);
    }
    if (count % 4 == 2)
    {
        file::copy("dummy3.json", "prod-data.json", cpOptions);
    }
    if (count % 4 == 3)
    {
        file::copy("dummy4.json", "prod-data.json", cpOptions);
    }
    if (count % 4 == 4)
    {
        file::copy("dummy5.json", "prod-data.json", cpOptions);
    }
}

int main(int argc, char * argv[])
{
    // copy files to /tmp to be persistent after reboot
    initTmp();

    // stop program execution, if "prod-data.json" doesn't satisfy the schema
    if (!schemaValidate())
    {
        exit(1);
    }

    // extract Thread network datasetkey
    string datasetkey = json::parse(std::ifstream("prod-data.json"))["datasetkey"].get_ref<std::string &>();
    if (datasetkey == "")
    {
        printf("Dataset key empty, please adjust prod-data.json !\n");
        exit(1);
    }

    /**
     * Pairing device, if programm execution looks like this:
     * ./controller pair <nodeid> <pincode> <discriminator>
     *
     * - doesn't necessarily have to be used
     *      - nodes can be manually added via the "chip-tool-release"
     *      - in this case "devices.json" has to be updated manually
     * */
    if (argc == 5 && strcmp(argv[1], "pair") == 0)
    {
        string nodeid        = string(argv[2]);
        string pincode       = string(argv[3]);
        string discriminator = string(argv[4]);
        pairNode(nodeid, pincode, discriminator, true, datasetkey);
        // copy files from /tmp to be persistent after reboot
        exitTmp();
    }
    /**
     * Pairing device, if programm execution looks like this:
     * ./controller unpair <nodeid>
     *
     * - doesn't necessarily have to be used
     *      - nodes can be manually deleted via the "chip-tool-release"
     *      - in this case "devices.json" has to be updated manually
     * */
    else if (argc == 3 && strcmp(argv[1], "unpair") == 0)
    {
        string nodeid = string(argv[2]);
        unpairNode(nodeid, true, datasetkey);
        // copy files from /tmp to be persistent after reboot
        exitTmp();
    }
    /**
     * Mode for testing / showcasing functionality
     * - updates nodes every 30 seconds with 3 different data sources
     *
     * program execution:
     * ./controller test-functionality
     * */
    else if (argc == 2 && strcmp(argv[1], "test-functionality") == 0)
    {
        int count = 0;
        while (1)
        {
            switchDummy(count);
            updateNodes();
            count++;
            sleep(60);
        }
    }
    /**
     * Standard update mode
     * - updates nodes every 15 minutes according to "prod-data.json"
     *
     * program execution:
     * ./controller
     * */
    else if (argc == 1)
    {
        while (1)
        {
            updateNodes();
            sleep(60 * UPDATE_INTERVAL_MINUTES);
        }
    }

    exit(0);
}
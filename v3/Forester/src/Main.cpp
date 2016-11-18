#include <cstdlib>
#include <set>
using std::set;
#include <ctime>
#include <iostream>
using std::cout;
using std::cin;
using std::endl;
using std::exit;
#include <iomanip> // For the display of inferred/refined subnets
using std::left;
using std::setw;
#include <fstream>
using std::ifstream;
using std::ofstream;
using std::ios;
#include <memory>
using std::auto_ptr;
#include <string>
using std::string;
#include <list>
using std::list;
using std::iterator;
#include <algorithm> // For transform() function
#include <getopt.h> // For options parsing
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <ctime> // To obtain current time (for output file)
#include <unistd.h> // For usleep() function

#include "common/inet/InetAddress.h"
#include "common/inet/InetAddressException.h"
#include "common/inet/NetworkAddress.h"
#include "common/inet/NetworkAddressSet.h"
#include "common/inet/InetAddressSet.h"
#include "common/thread/Thread.h"
#include "common/thread/Runnable.h"
#include "common/thread/Mutex.h"
#include "common/thread/ConditionVariable.h"
#include "common/thread/MutexException.h"
#include "common/thread/TimedOutException.h"
#include "common/date/TimeVal.h"
#include "common/utils/StringUtils.h"
#include "common/random/PRNGenerator.h"
#include "common/random/Uniform.h"

#include "treenet/TreeNETEnvironment.h"
#include "treenet/utils/SubnetParser.h"
#include "treenet/utils/IPDictionnaryParser.h"
#include "treenet/tree/NetworkTree.h"
#include "treenet/tree/growth/classic/ClassicGrower.h"
#include "treenet/tree/growth/graft/Grafter.h"
#include "treenet/tree/climbers/Robin.h"
#include "treenet/tree/climbers/Cuckoo.h"
#include "treenet/tree/climbers/Crow.h"
#include "treenet/tree/climbers/Cat.h"

const static unsigned short REDO_MODE_NOTHING = 0;
const static unsigned short REDO_MODE_ALIASES = 1;
const static unsigned short REDO_MODE_ALIAS_HINTS = 2;
const static unsigned short REDO_MODE_ROUTES = 3;

// Simple function to display usage.

void printInfo()
{
    cout << "Summary\n";
    cout << "=======\n";
    cout << "\n";
    cout << "TreeNET \"Forester\" is a companion software for TreeNET \"Arborist\". While the\n";
    cout << "Arborist version is designed exclusively for probing and active processing of\n";
    cout << "the collected data, the Forester version is designed to parse output files\n";
    cout << "from Arborist in order to merge several datasets (= several network trees)\n";
    cout << "together and/or re-do the alias resolution step, either by considering new\n";
    cout << "parameters, either by re-doing the probing work required for this step. The\n";
    cout << "Forester version is therefore able to merge partial datasets collected on a\n";
    cout << "large target network (or AS), potentially from several distinct vantage points\n";
    cout << "(or VPs), into a unified dataset and produce new output files which should\n";
    cout << "better model the target network.\n";
    cout << "\n";
    cout << "It is also possible to re-do the traceroute to each subnet, though this is not\n";
    cout << "the recommended solution for dataset merging. However, it can be very useful\n";
    cout << "if a measurement with Arborist crashed halfway, such that only subnets without\n";
    cout << "route were obtained.\n";
    cout << "\n";
    cout << "Like its probing brother, TreeNET \"Forester\" consists in a sequence of precise\n";
    cout << "algorithmic steps which are however not always carried out, as some steps only\n"; 
    cout << "occur if the user provides several datasets and/or uses particular options\n";
    cout << "(see usage). The different algorithmic steps are described below.\n";
    cout << "\n";
    cout << "Algorithmic steps\n";
    cout << "=================\n";
    cout << "\n";
    cout << "0) Launch and parsing\n";
    cout << "---------------------\n";
    cout << "\n";
    cout << "Forester parses options and flags (if any) and its main argument which should\n";
    cout << "list all the input files containing the data the user wants Forester to work\n";
    cout << "with. These input files shoud actually be seen more like prefixes of their\n";
    cout << "real names, as Forester will automatically append their names with the right\n";
    cout << "extensions (.subnet and .ip) to get all relevant parts of a given dataset for\n";
    cout << "the subsequent steps. If there are several datasets, they are parsed into\n";
    cout << "separate structures rather than a single one.\n";
    cout << "\n";
    cout << "1) (If several datasets) Grafting mode\n";
    cout << "--------------------------------------\n";
    cout << "\n";
    cout << "When several datasets are provided, Forester enters a special mode called\n";
    cout << "\"grafting mode\" where it proceeds to merge the .subnet files into a single,\n"; 
    cout << "unified file. The challenge of this step is to get accurate traceroute results\n";
    cout << "for each subnet in order to get a credible tree, which is made difficult by\n";
    cout << "the fact that each dataset probably comes from a different vantage point (or\n";
    cout << "VP). To obtain reasonable results, Forester isolates each datasets in\n";
    cout << "different trees and proceeds to select the best VP for grafting. This VP is\n";
    cout << "elected by computing, for each dataset, the percentage of subnets from other\n";
    cout << "datasets which have late labels (i.e., at the end of routes) in common with\n";
    cout << "the current dataset. Once selected, the routes in datasets from other VPs are\n";
    cout << "\"grafted\" using the common late labels.\n";
    cout << "\n";
    cout << "The edited subnets are written in a new .subnet output file. Subnets which the\n";
    cout << "route could not be grafted are considered as \"scraps\" and written in a\n";
    cout << "separate file (also a .subnet one, prefixed with \"Scraps \"). Forester then\n";
    cout << "stops its execution, but can be relaunched to complete the new .subnet file\n";
    cout << "with corresponding .ip, .alias and .fingerprint output files.\n";
    cout << "\n";
    cout << "This \"grafting\" process is currently Work In Progress, or rather, it has been \n";
    cout << "implemented but not yet validated. It also has the advantage of being entirely\n";
    cout << "passive.\n";
    cout << "\n";
    cout << "2) (Optional) Paris traceroute\n";
    cout << "------------------------------\n";
    cout << "\n";
    cout << "If asked by the user, Forester re-does the Paris traceroute step as done by\n";
    cout << "Arborist. However, this step is quite different than in Arborist, because the\n";
    cout << "potential change of vantage point (or VP) might result in completely different\n";
    cout << "distances (in TTL). Because of that, Forester does Paris traceroute by forward\n";
    cout << "probing to the pivot IP of a target subnet in order to estimate the distance\n";
    cout << "at the same time. It also double checks the contra-pivot IP(s) to re-position\n";
    cout << "the subnet if needed: changing of VP might also change the contra-pivot IP in\n";
    cout << "some situations.\n";
    cout << "\n";
    cout << "3) Network tree construction\n";
    cout << "----------------------------\n";
    cout << "\n";
    cout << "Forester builds the network tree with the data it got from parsing and Paris\n";
    cout << "traceroute (if the user asked the program to re-do it) with the same method as\n";
    cout << "used in Arborist. This step is entirely passive and is a pre-requisite for\n";
    cout << "alias resolution.\n";
    cout << "\n";
    cout << "4) (Optional) Alias resolution hints collection\n";
    cout << "-----------------------------------------------\n";
    cout << "\n";
    cout << "The network tree is used to isolate candidate IPs for alias resolution,\n";
    cout << "which are probed again to obtain alias resolution \"hints\" - data that will\n";
    cout << "help to fingerprint them and select the best alias resolution method to\n";
    cout << "associate together IPs bordering a same neighborhood.\n";
    cout << "\n";
    cout << "This step only occurs if the user explicitely asks the program to re-collect\n";
    cout << "these hints, rather than using the data available in the IP dictionnary. It\n";
    cout << "also produces a new .ip output file.\n";
    cout << "\n";
    cout << "5) (Optional) Alias resolution\n";
    cout << "------------------------------\n";
    cout << "\n";
    cout << "The data provided in the input files or collected at the previous step (if the\n";
    cout << "user asked for data re-collection) is used to carry out the actual alias\n";
    cout << "resolution, maybe with edited parameters (-x, -y and -z options, cf. usage),\n";
    cout << "if requested by the user via any of the non-default re-do modes. As a\n";
    cout << "consequence, Forester produces new .alias and .fingerprint output files. This\n";
    cout << "step is entirely passive.\n";
    cout << "\n";
    cout << "6) (Semi-optional) Network tree display and analysis\n";
    cout << "----------------------------------------------------\n";
    cout << "\n";
    cout << "Forester displays the structure of the tree in console. Unlike Arborist, this\n";
    cout << "step is systematical and not bound the display mode. Indeed, in Arborist, the\n";
    cout << "tree structure is only showed if the display mode is set to \"slightly verbose\"\n";
    cout << "or an even more verbose mode.\n";
    cout << "\n";
    cout << "However, the network tree in-depth analysis, in which every relevant\n";
    cout << "neighborhood is described and analyzed in details, is optional in Forester and\n";
    cout << "can be requested via an optional flag (see usage). This is because the typical\n";
    cout << "usage of Forester consists in fixing or completing existing data, rather than\n";
    cout << "analyzing it (which is the responsibility of Lumbrjack). This option is\n";
    cout << "nevertheless still relevant in some situations.\n";
    cout << "\n";
    
    cout.flush();
}

void printUsage()
{
    cout << "Usage\n";
    cout << "=====\n";
    cout << "\n";
    cout << "You can use Forester as follows:\n";
    cout << "\n";
    cout << "./treenet_forester [input file prefix n°1],[input file prefix n°2],[...]\n";
    cout << "\n";
    cout << "Here, an \"input file prefix\" denotes the name of any file from a dataset\n";
    cout << "obtained via Arborist without any suffix (.subnet, .ip, .alias, etc.). Indeed,\n";
    cout << "Forester will automatically append the right suffix for each file it should\n";
    cout << "parse.\n";
    cout << "\n";
    cout << "You can use various options and flags to handle the settings of Forester, such\n";
    cout << "as probing protocol (when there is probing), the amount of threads used in\n";
    cout << "multi-threaded parts of the application, etc. These options and flags are\n";
    cout << "detailed below.\n";
    cout << "\n";
    cout << "Short   Verbose                             Expected value\n";
    cout << "-----   -------                             --------------\n";
    cout << "\n";
    cout << "-m      --redo-mode                         0, 1, 2 or 3\n";
    cout << "\n";
    cout << "Use this option to have Forester re-do algorithmic steps as performed by\n";
    cout << "Arborist in order to complete and/or \"fix\" the provided data. Each possible\n";
    cout << "value corresponds to a particular mode.\n";
    cout << "\n";
    cout << "* 0: nothing happens (default setting). Forester will use the provided data\n";
    cout << "  \"as is\" and will not modify it in any way.\n";
    cout << "\n";
    cout << "* 1: re-do the \"true\" alias resolution, i.e., Forester will not modify the\n";
    cout << "  IP dictionnary and keep the provided alias resolution hints, but it will\n";
    cout << "  re-do the aliasing (possibly with new parameters, see -w, -x, -y and -z)\n";
    cout << "  and produce new .alias and .fingerprint output files. This mode allows the\n";
    cout << "  user to toy around with alias resolution parameters, with the exception of\n";
    cout << "  the amount of IP-IDs per IP.\n";
    cout << "\n";
    cout << "* 2: re-do the entire alias resolution process. Not only Forester will produce\n";
    cout << "  new .alias and .fingerprint output files juste like in the previous case,\n";
    cout << "  but it will also re-do the alias resolution hint collection via new probes.\n";
    cout << "  This is especially useful after grafting, since the grafting mode only\n";
    cout << "  produces a new .subnet file.\n";
    cout << "\n";
    cout << "* 3: re-do the traceroute to each subnet AND the entire alias resolution\n";
    cout << "  process. It is just like above, except that Forester also re-do the\n";
    cout << "  traceroute to each subnet prior to alias resolution. It is worth noting that\n";
    cout << "  traceroute in Forester is slightly different than in Arborist, because it\n";
    cout << "  has to re-evaluate distances (in TTL) to a pivot IP of a target subnet\n";
    cout << "  before the actual traceroute, as the current vantage point (or VP) is not\n";
    cout << "  necessarily the same as the one who measured the subnet. For the same\n";
    cout << "  reason, it also checks the contra-pivot interface(s) to re-position the IPs\n";
    cout << "  of the whole subnet if necessary.\n";
    cout << "\n";
    cout << "-n      --neighborhood-analysis             None (flag)\n";
    cout << "\n";
    cout << "Unlike Arborist, Forester does not systematically analyze the final network\n";
    cout << "tree to analyze each neighborhood. Add this flag to your command line to ask\n";
    cout << "for such an analysis at the end of the execution.\n";
    cout << "\n";
    cout << "-o      --parsing-omit-merging              None (flag)\n";
    cout << "\n";
    cout << "Because datasets can get quite large, checking at each newly parsed subnet\n";
    cout << "that there is a collision between the new subnet and any subnet of the set\n";
    cout << "being built (i.e., the former overlaps the latter), in which case both subnets\n";
    cout << "should be merged, can be a waste of time. Add this flag to your command line\n";
    cout << "to ask Forester to omit this verification.\n";
    cout << "\n"; 
    cout << "-e      --probing-egress-interface          IP or DNS\n";
    cout << "\n";
    cout << "Interface name through which probing/response packets exit/enter (default is\n";
    cout << "the first non-loopback IPv4 interface in the active interface list). Use this\n";
    cout << "option if your machine has multiple network interface cards and if you want to\n";
    cout << "prefer one interface over the others.\n";
    cout << "\n";
    cout << "-f      --probing-no-fixed-flow             None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to deactivate the \"Paris traceroute\"\n";
    cout << "technique used while probing. By default, Forester always uses it to ensure\n";
    cout << "that remote routers will always use the same path for a given destination,\n";
    cout << "such that the TTL to reach this destination is always the same.\n";
    cout << "\n";
    cout << "-p      --probing-payload-message           String\n";
    cout << "\n";
    cout << "Use this option to edit the message carried in the payload of the probes. This\n";
    cout << "message should just advertise that the probes are not sent for malicious\n";
    cout << "purposes, only for measurements. By default, it states \"NOT AN ATTACK\".\n";
    cout << "\n";
    cout << "-b      --probing-base-protocol             \"UDP\", \"TCP\" or \"ICMP\"\n";
    cout << "\n";
    cout << "Use this option to specify the base protocol used to probe target addresses.\n";
    cout << "By default, it is ICMP. Note that this is only the \"base\" protocol because\n";
    cout << "some inference techniques (like some alias resolution methods) rely on a\n";
    cout << "precise protocol which is therefore used instead.\n";
    cout << "\n";
    cout << "-r      --probing-regulating-period         Integer (amount of milliseconds)\n";
    cout << "\n";
    cout << "Use this option to edit the minimum amount of milliseconds between two\n";
    cout << "consecutive probes sent by a same thread. By default, this value is set to 50.\n";
    cout << "\n";
    cout << "-s      --probing-second-opinion            None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to always probe a target a second time when\n";
    cout << "the first probe results in a failure. Probing a target a second time can\n";
    cout << "mitigate failures over short periods of time, but if the target network is\n";
    cout << "unreliable (i.e., very variable response delays), it can slow down the whole\n";
    cout << "application.\n";
    cout << "\n";
    cout << "-t      --probing-timeout-period            Integer (amount of milliseconds)\n";
    cout << "\n";
    cout << "Use this option to edit the amount of milliseconds spent waiting before\n";
    cout << "diagnosing a timeout. Editing this value will especially impact the network\n";
    cout << "pre-scanning phase (i.e., phase where the liveness of target IPs is checked).\n";
    cout << "By default, this value is set to 2500 (2,5 seconds).\n";
    cout << "\n";
    cout << "-a      --concurrency-amount-threads        Integer (amount of threads)\n";
    cout << "\n";
    cout << "Use this option to edit the amount of threads used during any multi-threaded\n";
    cout << "part of Forester (such as alias resolution hints collection, if asked). By\n";
    cout << "default, this value is set to 256.\n";
    cout << "\n";
    cout << "WARNING: due to scheduling strategies used during alias resolution, this value\n";
    cout << "CANNOT be smaller than the amount of collected IP IDs per IP (from [3,20])\n";
    cout << "plus one. Using less threads would lead to malfunction and less efficient\n";
    cout << "alias resolution.\n";
    cout << "\n";
    cout << "-d      --concurrency-delay-threading       Integer (amount of milliseconds)\n";
    cout << "\n";
    cout << "Use this option to edit the minimum amount of milliseconds spent waiting\n";
    cout << "between two immediately consecutive threads. This value is set to 250 (i.e.,\n";
    cout << "0,25s) by default. This delay is meant to avoid sending too many probes at the\n";
    cout << "same time in parts of Forester involving multi-threading, such as alias\n";
    cout << "resolution hint collection. Indeed, a small delay for a large amount of\n";
    cout << "threads could potentially cause congestion and make the whole application\n";
    cout << "ineffective.\n";
    cout << "\n";
    cout << "-w      --alias-resolution-amount-ip-ids    Integer from [3,20]\n";
    cout << "\n";
    cout << "Use this option to edit the amount of IP IDs that are collected for each IP\n";
    cout << "that is candidate for alias resolution. By default, this amount is set to 4.\n";
    cout << "Increasing this value will have an impact on the viability of Ally technique\n";
    cout << "and the accuracy while estimating the velocity range of the IP ID counter\n";
    cout << "associated with a given IP, at the cost of sending more probes. You cannot\n";
    cout << "assign a value smaller than 3 because there needs at least two time intervals\n";
    cout << "between 3 consecutive IP IDs to get a range.\n";
    cout << "\n";
    cout << "WARNINGS:\n";
    cout << "-due to scheduling strategies used during alias resolution, this value SHOULD\n";
    cout << "NOT be greater than the amount of threads being used minus one. Using less\n";
    cout << "threads would lead to malfunction and less efficient alias resolution.\n";
    cout << "-this amount is also considered while parsing an IP dictionnary. If a line\n";
    cout << "contains more IP-IDs than specified, the exceeding IDs will not be considered.\n";
    cout << "Reciprocally, a line with less IP-IDs than specified will not be fully parsed.\n";
    cout << "\n";
    cout << "-x      --alias-resolution-rollovers-max    Integer from [1,256]\n";
    cout << "\n";
    cout << "Use this option to edit the maximum amount of rollovers considered while\n";
    cout << "estimating the speed at which an IP ID counter evolves. This, of course, is\n";
    cout << "only relevant for the alias resolution technique based on velocity. By\n";
    cout << "default, Forester considers a maximum of 10.\n";
    cout << "\n";
    cout << "-y      --alias-resolution-range-tolerance  Positive, non-zero real value\n";
    cout << "\n";
    cout << "Use this option to edit the tolerance value for checking velocity range\n";
    cout << "overlap while using the velocity-based alias resolution technique. Such value\n";
    cout << "is used to extend the largest interval between two intervals being compared\n";
    cout << "so that very close, yet non-overlapping intervals (say, [0.5,0.55] and\n";
    cout << "[0,56, 0.6]) can be associated together. By default, it is 0.2. Increasing\n";
    cout << "this value might, however, increase the amount of false positives while\n";
    cout << "aliasing IPs together.\n";
    cout << "\n";
    cout << "-z      --alias-resolution-error-tolerance  Real from [0,0.5[\n";
    cout << "\n";
    cout << "Use this option to edit the tolerance value for rouding errors while\n";
    cout << "estimating the velocity of an IP ID counter. After computing the potential\n";
    cout << "values for rollovers that happened between each pair of consecutive IP IDs\n";
    cout << "as real values, the error after rounding them to get integer values should\n";
    cout << "stay below a threshold to ensure the soundness of the values. By default, this\n";
    cout << "threshold is set to 0.35.\n";
    cout << "\n";
    cout << "-l      --label-output                      String\n";
    cout << "\n";
    cout << "Use this option to edit the label that will be used to name the various output\n";
    cout << "files produced by Forester (usually ended with a suffix related to their\n";
    cout << "content: .subnet for the subnet dump, .ip for the IP dictionnary with alias\n";
    cout << "resolution hints, etc.). By default, Forester will use the time at which it\n";
    cout << "needs to output a file (for instance, after alias resolution hint collection)\n";
    cout << "in the format dd-mm-yyyy hh:mm:ss.\n";
    cout << "\n";
    cout << "-v      --verbosity                         0, 1 or 2\n";
    cout << "\n";
    cout << "Use this option to handle the verbosity of the console output produced by\n";
    cout << "Forester. Each accepted value corresponds to a \"mode\":\n";
    cout << "\n";
    cout << "* 0: it is the default verbosity. In this mode, Forester only displays the\n";
    cout << "  most significant details about progress. For instance, during the alias\n";
    cout << "  resolution hint collection, it only displays the neighborhood for which the\n";
    cout << "  hints are being computed, but it does not advertise the detailed steps.\n";
    cout << "\n";
    cout << "* 1: this is the \"verbose\" mode. Forester displays more algorithmic details\n";
    cout << "  and results. For instance, it displays the different steps of the alias\n";
    cout << "  resolution hint collection.\n";
    cout << "\n";
    cout << "* 2: again stacking up on the previous, this last mode also dumps small logs\n";
    cout << "  for each probe once it has been carried out. It is equivalent to a debug\n";
    cout << "  mode.\n";
    cout << "\n";
    cout << "  WARNING: this debug mode is extremely verbose and should only be considered\n";
    cout << "  for unusual situations where investigating the result of each probe becomes\n";
    cout << "  necessary. Do not use it for large-scale work, like the merging of several\n";
    cout << "  datasets collected from a large AS with alias resolution hints collection,\n";
    cout << "  as redirecting the console output to files might produce unnecessarily large\n";
    cout << "  files.\n";
    cout << "\n";
    cout << "-c      --credits                           None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to display the version of Forester and some\n";
    cout << "credits. Forester will not run further in this case, though -h and -i flags can\n";
    cout << "be used in addition.\n";
    cout << "\n";
    cout << "-h      --help                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to know how to use Forester and see the\n";
    cout << "complete list of options and flags and how they work. Forester will not run\n";
    cout << "further after displaying this, though -c and -i flags can be used in addition.\n";
    cout << "\n";
    cout << "-i      --info                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to read a summary of how Forester works,\n";
    cout << "with a list of the different algorithm steps. Forester will not run\n";
    cout << "further after displaying this, though -c and -h flags can be used in addition.\n";
    cout << "\n";
    
    cout.flush();
}

void printVersion()
{
    cout << "Version and credits\n";
    cout << "===================\n";
    cout << "\n";
    cout << "TreeNET v3.0 \"Forester\", written by Jean-François Grailet (10 - 11/2016).\n";
    cout << "Based on ExploreNET version 2.1, copyright (c) 2013 Mehmet Engin Tozal.\n";
    cout << "\n";
    
    cout.flush();
}

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Default parameters (can be edited by user)
    InetAddress localIPAddress;
    unsigned short probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_ICMP;
    string probeAttentionMessage = string("NOT an ATTACK (mail: Jean-Francois.Grailet@ulg.ac.be)");
    TimeVal timeoutPeriod(2, TimeVal::HALF_A_SECOND); // 2s + 500 000 microseconds = 2,5s
    TimeVal probeRegulatingPeriod(0, 50000); // 0,05s
    TimeVal probeThreadDelay(0, 250000); // 0,25s
    unsigned short nbIPIDs = 4; // For Alias Resolution
    unsigned short maxRollovers = 10; // Idem
    double baseTolerance = 0.2; // Idem
    double maxError = 0.35; // Idem
    bool doubleProbe = false;
    bool useFixedFlowID = true;
    unsigned short redoMode = REDO_MODE_NOTHING;
    bool neighborhoodAnalysis = false;
    bool parsingOmitMerging = false;
    unsigned short displayMode = TreeNETEnvironment::DISPLAY_MODE_LACONIC;
    unsigned short nbThreads = 256;
    string labelOutputFiles = ""; // Gets a default value later if not set by user.
    
    // Values to check if info, usage, version... should be displayed.
    bool displayInfo = false, displayUsage = false, displayVersion = false;
    
    /*
     * PARSING ARGUMENT
     * 
     * The main argument (input files, separated by commas) can be located anywhere. To make 
     * things simple for getopt_long(), argv is processed to find it and put it at the end. If not 
     * found, the program stops and displays an error message.
     */
    
    int totalArgs = argc;
    string inputsStr = ""; // List of input files
    bool found = false;
    bool flagParam = false; // True if a value for a parameter is expected
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'c':
                case 'f':
                case 'h':
                case 'i':
                case 'n':
                case 's':
                    break;
                default:
                    flagParam = true;
                    break;
            }
        }
        else if(flagParam)
        {
            flagParam = false;
        }
        else
        {
            // Argument found!
            char *arg = argv[i];
            for(int j = i; j < argc - 1; j++)
                argv[j] = argv[j + 1];
            argv[argc - 1] = arg;
            found = true;
            totalArgs--;
            break;
        }
    }
    
    inputsStr = argv[argc - 1];   
    
    /*
     * PARSING PARAMETERS
     *
     * In addition to the main argument parsed above, Forester provides various input flags which 
     * can be used to handle the probing parameters, concurrency parameters, alias resolution 
     * parameters and a few more features (like setting the message sent with the ICMP probes and 
     * the label of the output files).
     */
     
    int opt = 0;
    int longIndex = 0;
    const char* const shortOpts = "a:b:cd:e:fhil:m:nop:r:st:v:w:x:y:z:";
    const struct option longOpts[] = {
            {"redo-mode", required_argument, NULL, 'm'}, 
            {"neighborhood-analysis", no_argument, NULL, 'n'}, 
            {"parsing-omit-merging", no_argument, NULL, 'o'}, 
            {"probing-egress-interface", required_argument, NULL, 'e'}, 
            {"probing-no-fixed-flow", no_argument, NULL, 'f'}, 
            {"probing-payload-message", required_argument, NULL, 'p'}, 
            {"probing-base-protocol", required_argument, NULL, 'b'}, 
            {"probing-regulating-period", required_argument, NULL, 'r'}, 
            {"probing-second-opinion", no_argument, NULL, 's'}, 
            {"probing-timeout-period", required_argument, NULL, 't'}, 
            {"concurrency-amount-threads", required_argument, NULL, 'a'}, 
            {"concurrency-delay-threading", required_argument, NULL, 'd'}, 
            {"alias-resolution-amount-ip-ids", required_argument, NULL, 'w'}, 
            {"alias-resolution-rollovers-max", required_argument, NULL, 'x'}, 
            {"alias-resolution-range-tolerance", required_argument, NULL, 'y'}, 
            {"alias-resolution-error-tolerance", required_argument, NULL, 'z'}, 
            {"label-output", required_argument, NULL, 'l'}, 
            {"verbosity", required_argument, NULL, 'v'}, 
            {"credits", no_argument, NULL, 'c'}, 
            {"help", no_argument, NULL, 'h'}, 
            {"info", no_argument, NULL, 'i'}
    };
    
    string optargSTR;
    unsigned long val;
    unsigned long sec;
    unsigned long microSec;
    double valDouble;
    
    try
    {
        while((opt = getopt_long(totalArgs, argv, shortOpts, longOpts, &longIndex)) != -1)
        {
            /*
             * Beware: use the line optargSTR = string(optarg); ONLY for flags WITH arguments !! 
             * Otherwise, it prevents the code from recognizing flags like -v, -h or -g (because 
             * they require no argument) and make it throw an exception... To avoid this, a second 
             * switch is used.
             *
             * (this is noteworthy, as this error is still present in ExploreNET v2.1)
             */
            
            switch(opt)
            {
                case 'c':
                case 'f':
                case 'h':
                case 'i':
                case 'n':
                case 'o':
                case 's':
                    break;
                default:
                    optargSTR = string(optarg);
                    
                    /*
                     * For future readers: optarg is of type extern char*, and is defined in getopt.h.
                     * Therefore, you will not find the declaration of this variable in this file.
                     */
                    
                    break;
            }
            
            // Now we can actually treat the options.
            int gotNb = 0;
            switch(opt)
            {
                case 'm':
                    gotNb = std::atoi(optargSTR.c_str());
                    if(gotNb >= 0 && gotNb <= 3)
                        redoMode = (unsigned short) gotNb;
                    else
                    {
                        cout << "Warning for -m option: an unrecognized mode (i.e., value ";
                        cout << "out of [0,3]) was provided. Forester will not re-compute ";
                        cout << "anything.\n" << endl;
                    }
                    break;
                case 'n':
                    neighborhoodAnalysis = true;
                    break;
                case 'o':
                    parsingOmitMerging = true;
                    break;
                case 'e':
                    try
                    {
                        localIPAddress = InetAddress::getLocalAddressByInterfaceName(optargSTR);
                    }
                    catch (InetAddressException &e)
                    {
                        cout << "Error for -e option: cannot obtain any IP address ";
                        cout << "assigned to the interface \"" + optargSTR + "\". ";
                        cout << "Please fix the argument for this option before ";
                        cout << "restarting Forester.\n" << endl;
                        return 1;
                    }
                    break;
                case 'f':
                    useFixedFlowID = false;
                    break;
                case 'p':
                    probeAttentionMessage = optargSTR;
                    break;
                case 'b':
                    std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                    if(optargSTR == string("UDP"))
                        probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_UDP;
                    else if(optargSTR == string("TCP"))
                        probingProtocol = TreeNETEnvironment::PROBING_PROTOCOL_TCP;
                    else if(optargSTR != string("ICMP"))
                    {
                        cout << "Warning for option -b: unrecognized protocol " << optargSTR;
                        cout << ". Please select a protocol between the following three: ";
                        cout << "ICMP, UDP and TCP. Note that ICMP is the default base ";
                        cout << "protocol.\n" << endl;
                    }
                    break;
                case 'r':
                    val = 1000 * StringUtils::string2Ulong(optargSTR);
                    if(val > 0)
                    {
                        sec = val / TimeVal::MICRO_SECONDS_LIMIT;
                        microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
                        probeRegulatingPeriod.setTime(sec, microSec);
                    }
                    else
                    {
                        cout << "Warning for -r option: a negative value (or 0) was parsed. ";
                        cout << "Forester will use the default value for the probe regulating ";
                        cout << "period (= 50ms).\n" << endl;
                    }
                    break;
                case 's':
                    doubleProbe = true;
                    break;
                case 't':
                    val = 1000 * StringUtils::string2Ulong(optargSTR);
                    if(val > 0)
                    {
                        sec = val / TimeVal::MICRO_SECONDS_LIMIT;
                        microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
                        timeoutPeriod.setTime(sec, microSec);
                    }
                    else
                    {
                        cout << "Warning for -r option: a negative value (or 0) was parsed. ";
                        cout << "Forester will use the default value for the timeout period ";
                        cout << "(= 2,5s).\n" << endl;
                    }
                    break;
                case 'a':
                    gotNb = std::atoi(optargSTR.c_str());
                    if (gotNb > 0 && gotNb < 32767)
                    {
                        nbThreads = (unsigned short) gotNb;
                    }
                    else
                    {
                        cout << "Warning for -a option: a value smaller than 1 or greater ";
                        cout << "than 32766 was parsed. Forester will use the default amount ";
                        cout << "of threads (= 256).\n" << endl;
                        break;
                    }
                    
                    if (gotNb < (nbIPIDs + 1))
                    {
                        nbThreads = 256;
                        
                        cout << "Warning for -a option: a value that is positive but smaller ";
                        cout << "than the amount of IP IDs being collected per IP plus one was ";
                        cout << "parsed. Such a configuration is forbidden because of the ";
                        cout << "scheduling strategies used in the alias resolution step for ";
                        cout << "IP ID collection. Forester will use the default amount of ";
                        cout << "threads (= 256).\n" << endl;
                    }
                    
                    break;
                case 'd':
                    val = 1000 * StringUtils::string2Ulong(optargSTR);
                    if(val > 0)
                    {
                        sec = val / TimeVal::MICRO_SECONDS_LIMIT;
                        microSec = val % TimeVal::MICRO_SECONDS_LIMIT;
                        probeThreadDelay.setTime(sec, microSec);
                    }
                    else
                    {
                        cout << "Warning for -d option: a negative value (or 0) was parsed. ";
                        cout << "Forester will use the default value for the delay between the ";
                        cout << "launch of two consecutive threads (= 250ms).\n" << endl;
                    }
                    break;
                case 'w':
                    gotNb = std::atoi(optargSTR.c_str());
                    if (gotNb > 2 && gotNb <= 20)
                    {
                        nbIPIDs = (unsigned short) gotNb;
                    }
                    else
                    {
                        cout << "Warning for -w option: a value outside the suggested range ";
                        cout << "(i.e., [3,20]) was provided. Forester will use the default ";
                        cout << "value for this option (= 4).\n" << endl;
                        break;
                    }
                    
                    if ((gotNb + 1) > nbThreads)
                    {
                        nbIPIDs = nbThreads - 1;
                        
                        cout << "Warning for -w option: a value that is positive but greater ";
                        cout << "than the amount of concurrent threads was parsed. Such a ";
                        cout << "configuration is forbidden because of the scheduling strategies ";
                        cout << "used in the alias resolution step for IP ID collection. ";
                        cout << "Forester will use the current amount of threads minus one for ";
                        cout << "the amount of collected IP IDs per IP.\n" << endl;
                    }
                    
                    break;
                case 'x':
                    gotNb = std::atoi(optargSTR.c_str());
                    if (gotNb > 0 && gotNb <= 256)
                    {
                        maxRollovers = (unsigned short) gotNb;
                    }
                    else
                    {
                        cout << "Warning for -x option: a value outside the suggested range ";
                        cout << "(i.e., [1,256]) was provided. Forester will use the default ";
                        cout << "value for this option (= 10).\n" << endl;
                    }
                    break;
                case 'y':
                    valDouble = StringUtils::string2double(optargSTR);
                    if(valDouble >= 0.0)
                    {
                        baseTolerance = valDouble;
                    }
                    else
                    {
                        cout << "Warning for -y option: a negative value was provided. Forester ";
                        cout << "will use the default value for this option (= 0.2).\n" << endl;
                    }
                    break;
                case 'z':
                    valDouble = StringUtils::string2double(optargSTR);
                    if(valDouble >= 0.0 && valDouble < 0.5)
                    {
                        maxError = valDouble;
                    }
                    else
                    {
                        cout << "Warning for -x option: a value outside the suggested range ";
                        cout << "(i.e., [0,0.5[) was provided. Forester will use the default ";
                        cout << "value for this option (= 0.35).\n" << endl;
                    }
                    break;
                case 'l':
                    labelOutputFiles = optargSTR;
                    break;
                case 'v':
                    gotNb = std::atoi(optargSTR.c_str());
                    if(gotNb >= 0 && gotNb <= 2)
                        displayMode = (unsigned short) gotNb;
                    else
                    {
                        cout << "Warning for -v option: an unrecognized mode (i.e., value ";
                        cout << "out of [0,2]) was provided. Forester will use the laconic ";
                        cout << "mode (default mode).\n" << endl;
                    }
                    break;
                case 'c':
                    displayVersion = true;
                    break;
                case 'h':
                    displayUsage = true;
                    break;
                case 'i':
                    displayInfo = true;
                    break;
                default:
                    break;
            }
        }
    }
    catch(std::logic_error &le)
    {
        cout << "Use -h or --help to get more details on how to use TreeNET." << endl;
        return 1;
    }
    
    if(displayInfo || displayUsage || displayVersion)
    {
        if(displayInfo)
            printInfo();
        if(displayUsage)
            printUsage();
        if(displayVersion)
            printVersion();
        return 0;
    }
    
    if(!found)
    {
        cout << "No input file was provided. Use -h or --help to get more ";
        cout << "details on how to use TreeNET \"Forester\"." << endl;
        return 0;
    }
    
    /*
     * SETTING THE ENVIRONMENT
     *
     * Before listing target IPs, the initialization of Forester is completed by getting the local 
     * IP (if needed) and creating a TreeNETEnvironment object, a singleton which will be passed 
     * by pointer to other classes of the program to be able to get all the current settings, 
     * which are either default values either values parsed in the parameters provided by the 
     * user. The singleton also provides access to data structures other classes should be able to 
     * access.
     */

    if(localIPAddress.isUnset() && redoMode != REDO_MODE_NOTHING)
    {
        try
        {
            localIPAddress = InetAddress::getFirstLocalAddress();
        }
        catch(InetAddressException &e)
        {
            cout << "TreeNET cannot obtain a valid local IP address for probing.\n";
            cout << "Please check the connectivity of your device before re-running it." << endl;
            return 1;
        }
    }
    
    /*
     * The code now checks if we can open a socket right now if we have to re-compute alias 
     * resolution hints and routes (or just alias resolution hints) to properly advertise the 
     * user should use "sudo" or "su". Indeed, many operations will occur before any new probing, 
     * so it's better to check now that we can do it rather than waiting for an emergency stop 
     * later in the program.
     */
    
    if(redoMode >= REDO_MODE_ALIAS_HINTS)
    {
        try
        {
            DirectProber *test = new DirectICMPProber(probeAttentionMessage, 
                                                      timeoutPeriod, 
                                                      probeRegulatingPeriod, 
                                                      DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER, 
                                                      DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER, 
                                                      DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE, 
                                                      DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE, 
                                                      false);
            
            delete test;
        }
        catch(SocketException e)
        {
            cout << "Unable to create sockets. Try running TreeNET as a privileged user (for ";
            cout << "example, try with sudo)." << endl;
            return 1;
        }
    }
       
    /*
     * If relevant (i.e. in case of route/alias resolution hints re-collection, grafting mode or 
     * emergency stop), the label for the new output files is determined here for convenience. It 
     * is either the label selected by the user via -l flag, either the current time in the format 
     * dd-mm-YYYY hh:mm:ss.
     */
    
    string newFileName = "";
    if(labelOutputFiles.length() > 0)
    {
        newFileName = labelOutputFiles;
    }
    else
    {
        // Get the current time for the name of output files (if any)
        time_t rawTime;
        struct tm *timeInfo;
        char buffer[80];

        time(&rawTime);
        timeInfo = localtime(&rawTime);

        // Name for the output file containing the list of subnets
        strftime(buffer, 80, "%d-%m-%Y %T", timeInfo);
        string timeStr(buffer);
        newFileName = timeStr;
    }

    // Initialization of the environment
    TreeNETEnvironment *env = new TreeNETEnvironment(&cout, 
                                                     !parsingOmitMerging, 
                                                     probingProtocol, 
                                                     doubleProbe, 
                                                     useFixedFlowID, 
                                                     localIPAddress, 
                                                     probeAttentionMessage, 
                                                     timeoutPeriod, 
                                                     probeRegulatingPeriod, 
                                                     probeThreadDelay, 
                                                     nbIPIDs, 
                                                     maxRollovers, 
                                                     baseTolerance, 
                                                     maxError, 
                                                     displayMode, 
                                                     nbThreads);
    
    // Gets quick access to subnet set
    SubnetSiteSet *set = env->getSubnetSet();
    
    // Some welcome message
    cout << "TreeNET v3.0 \"Forester\"\n" << endl;
    
    /*
     * INPUT FILE PARSING
     *
     * The provided input file(s) (either a single subnet dump, either a pair subnet dump/IP 
     * dictionnary dump) are read and parsed into the subnet set and the IP dictionnary of the 
     * current instance of TreeNET "Forester".
     */

    cout << "Parsing input file(s)...\n" << endl;

    // Listing input dump file(s)
    bool graftingMode = false; // Triggered when multiple files are input
    list<string> filePaths; // Needs to be declared here for later
    
    size_t pos = inputsStr.find(',');
    if(pos != std::string::npos)
    {
        // Listing all file paths
        std::stringstream ss(inputsStr);
        std::string inputFileStr;
        while (std::getline(ss, inputFileStr, ','))
            filePaths.push_back(inputFileStr);
    
        // Checking that each file exists
        for(list<string>::iterator it = filePaths.begin(); it != filePaths.end(); ++it)
        {
            // Subnet dump
            ifstream inFile;
            inFile.open(((*it) + ".subnet").c_str());
            if(inFile.is_open())
            {
                inFile.close();
            }
            else
            {
                cout << "No " << (*it) << ".subnet to parse.\n" << endl;
                filePaths.erase(it--);
            }
        }
        
        if(filePaths.size() > 1)
        {
            graftingMode = true;
        }
        else if(filePaths.size() == 1)
        {
            cout << "Multiple input files were provided, but only one of them actually ";
            cout << "exists in the file system. Grafting will not occur.\n" << endl;
            inputsStr = filePaths.front();
        }
        else
        {
            cout << "Please input existing TreeNET dump files (minus .subnet or .ip ";
            cout << "extension) to continue.\n" << endl;
            throw InvalidParameterException();
        }
    }
    
    // Single input file: both .subnet and .ip dumps are parsed
    if(!graftingMode)
    {
        // Parsing utilities
        SubnetParser *sp = new SubnetParser(env);
        IPDictionnaryParser *idp = new IPDictionnaryParser(env);
    
        // Subnet dump
        string subnetDumpContent = "";
        ifstream inFileSubnet;
        inFileSubnet.open((inputsStr + ".subnet").c_str());
        bool subnetDumpOk = false;
        if(inFileSubnet.is_open())
        {
            subnetDumpContent.assign((std::istreambuf_iterator<char>(inFileSubnet)),
                                     (std::istreambuf_iterator<char>()));
            
            inFileSubnet.close();
            subnetDumpOk = true;
        }
        else
        {
            cout << "No " << inputsStr << ".subnet to parse.\n" << endl;
        }
        
        if(subnetDumpOk)
        {
            // IP dictionnary dump
            string IPDumpContent = "";
            ifstream inFileIP;
            inFileIP.open((inputsStr + ".ip").c_str());
            bool IPDumpOk = false;
            if(inFileIP.is_open())
            {
                IPDumpContent.assign((std::istreambuf_iterator<char>(inFileIP)),
                                     (std::istreambuf_iterator<char>()));
                
                inFileIP.close();
                IPDumpOk = true;
            }
            
            // Code also checks the amount of subnets before/after parsing
            size_t curNbSubnets = set->getSubnetSiteList()->size();
            cout << "Parsing " << inputsStr << ".subnet..." << endl;
            sp->parse(set, subnetDumpContent, !IPDumpOk);
            cout << "Parsing of " << inputsStr << ".subnet completed.\n" << endl;
            size_t newNbSubnets = set->getSubnetSiteList()->size();
            
            if(newNbSubnets > curNbSubnets)
            {
                cout << "Parsing " << inputsStr << ".ip..." << endl;
                idp->parse(IPDumpContent);
                cout << "Parsing of " << inputsStr << ".ip completed.\n" << endl;
            }
            else if(IPDumpOk)
            {
                cout << "No subnet were found. " << inputsStr << ".ip parsing has been ";
                cout << "skipped.\n" << endl;
            }
            else
            {
                cout << "No " << inputsStr << ".ip to parse. IP Dictionnary has been ";
                cout << "partially completed with the IPs mentioned in ";
                cout << inputsStr << ".subnet.\n" << endl;
            }
        }
        
        delete sp;
        delete idp;
    }
    else
    {
        /*
         * GRAFTING MODE
         *
         * When several files are recognized, TreeNET "Forester" enter this mode to properly merge 
         * the different pieces of data together, such that a unique tree can be built. Indeed, it 
         * is very likely these pieces were collected from distinct vantage points (or VPs), and 
         * therefore, their routes are not suited for building a whole tree with all sets 
         * together. To overcome this issue, Forester performs a "grafting" operation which 
         * predicts the route that would be obtained from a particular VP (pre-selected to 
         * have a maximum of adapted routes) to some subnet, relying on the last interfaces 
         * occurring in its route. The intuition is that, no matter the VP, the last interface(s) 
         * towards a subnet should stay the same.
         */
         
        cout << "Multiple input files were recognized. TreeNET Reader will now proceed to ";
        cout << "merge them together into a single dataset.\n" << endl;
        
        try
        {
            Grafter *grafter = new Grafter(env, filePaths);
            grafter->selectRootstock();
            
            Soil *result = grafter->growAndGraft();
            
            // Simple tree display
            Climber *robin = new Robin(env);
            robin->climb(result);
            delete robin;
            
            // Save subnets with adapted routes
            result->outputSubnets(newFileName + ".subnet");
            cout << "Merged subnet sets with adapted routes have been saved in ";
            cout << newFileName << ".subnet." << endl;
            
            // Save scraps
            if(grafter->hasScrappedSubnets())
            {
                grafter->outputScrappedSubnets("Scrapped "+ newFileName + ".subnet");
                cout << "Scrapped subnets (i.e., they could not be grafted) have been saved in ";
                cout << "an output file \"Scrapped " << newFileName << ".subnet\".\n" << endl;
            }
            
            cout << "Grafting ended. TreeNET Forester will stop running here.\n\n";
            cout << "It is strongly recommended to re-run TreeNET Forester with the new dataset ";
            cout << "as input to re-collect alias resolution hints before analyzing the data.";
            cout << endl;
            
            delete grafter;
            delete result;
            delete env;
            return 0;
        }
        catch(BadInputException &bie)
        {
            delete env;
            return 1;
        }
    }
        
    // Outputting results with the set (simple display).
    cout << left << setw(25) << "Subnet";
    cout << left << setw(15) << "Class" << endl;
    cout << left << setw(25) << "------";
    cout << left << setw(15) << "-----" << endl;
    
    list<SubnetSite*> *ssList = set->getSubnetSiteList();
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        cout << left << setw(25) << (*it)->getInferredNetworkAddressString();
        cout << left << setw(15);
        switch((*it)->getStatus())
        {
            case SubnetSite::ACCURATE_SUBNET:
                cout << "Accurate";
                break;
            case SubnetSite::ODD_SUBNET:
                cout << "Odd";
                break;
            case SubnetSite::SHADOW_SUBNET:
                cout << "Shadow";
                break;
            default:
                cout << "Undefined";
                break;
        }
        cout << endl;
    }
    
    Grower *g = NULL;
    Climber *cuckoo = NULL;
    Soil *result = NULL;
    try
    {
        /*
         * SUBNET NEIGHBORHOOD INFERENCE
         *
         * Before inferring L2/L3 devices, we locate subnets regarding each other with a tree. 
         * The process of constructing the tree, as of September 2016, occurs in two steps: first 
         * step is the preparation, which is a preliminary probing task to collect additionnal 
         * details on the subnets and their location (for example, running Paris traceroute). 
         * Then the construction (or tree growth) actually starts, using these preliminary pieces 
         * of info. The construction itself can be enterily passive (i.e., no probing) or involve 
         * additionnal probing work.
         */
         
        g = new ClassicGrower(env);
        
        /*
         * The "preparation" is normally already done if the dataset is complete, so this step 
         * amounts in Forester to route re-computation.
         */
        
        if(redoMode >= REDO_MODE_ROUTES)
        {
            cout << "\nPreparing the tree growth...\n" << endl;
            g->prepare();
            cout << "Preparation complete." << endl;
        }
        
        cout << "\nGrowing network tree..." << endl;
        
        g->grow();
        
        // End of tree growth
        cout << "Growth complete.\n" << endl;
        
        result = g->getResult();
        delete g;
        g = NULL;
        
        // Simple tree display
        Climber *robin = new Robin(env);
        robin->climb(result);
        delete robin;
        
        /*
         * ALIAS RESOLUTION
         *
         * Interfaces bordering a neighborhood are probed again several times (but with a fixed 
         * amount of probes) to gather "alias resolution hints", i.e., pieces of data which will 
         * be used to profile the behavior of that IP via a fingerpriting process. Then, IPs 
         * of a same neighborhood showing a similar profile are gathered together in an offline 
         * post-processing of the tree to conduct actual alias resolution via the best suited 
         * method. The hints are sometimes involved in this (for instance, IP-IDs), though some 
         * profiles might not be compatible with any technique implemented in TreeNET.
         *
         * In Forester, both steps of the alias resolution (i.e., hint collection and actual 
         * resolution) are optional. The first is conducted only under two re-do modes, and the 
         * second only occurs if the user explicitely asks for it through a re-do mode or a 
         * detailed analysis of the tree (-n flag).
         */
        
        // Collects alias resolution hints.
        if(redoMode >= REDO_MODE_ALIAS_HINTS)
        {
            cout << "Collecting new alias resolution hints...\n" << endl;
            
            env->getIPTable()->clearAliasHints();
            
            cuckoo = new Cuckoo(env);
            cuckoo->climb(result);
            delete cuckoo;
            cuckoo = NULL;
            
            if(displayMode < TreeNETEnvironment::DISPLAY_MODE_DEBUG)
                cout << endl; // Unnecessary outside debug mode.
        }
        
        // Re-does alias resolution (+ save in output file if asked).
        Climber *crow = new Crow(env);
        crow->climb(result);
        if(redoMode >= REDO_MODE_ALIASES)
            ((Crow *) crow)->outputAliases(newFileName + ".alias");
        delete crow;
        
        // Internal node exploration and actual alias resolution are only done now.
        if(neighborhoodAnalysis)
        {
            Climber *cat = new Cat(env);
            cat->climb(result);
            delete cat;
        }
        
        // Outputs the new files and/or advertises they have been produced.
        if(redoMode >= REDO_MODE_ROUTES)
        {
            result->outputSubnets(newFileName + ".subnet");
            cout << "Inferred subnets with new routes have been saved in ";
            cout << newFileName << ".subnet." << endl;
        }
        
        if(redoMode >= REDO_MODE_ALIASES)
        {
            cout << "Newly inferred alias lists have been saved in an output file ";
            cout << newFileName << ".alias." << endl;
        }
        
        if(redoMode >= REDO_MODE_ALIAS_HINTS)
        {
            env->getIPTable()->outputDictionnary(newFileName + ".ip");
            cout << "IP dictionnary with new alias resolution hints has been saved in an output file ";
            cout << newFileName << ".ip." << endl;
        }
        
        delete result;
        result = NULL;
    }
    catch(StopException &e)
    {
        cout << "TreeNET is halting now.\n" << endl;
    
        if(set->getSubnetSiteList()->size() > 0)
        {
            set->outputAsFile("[Stopped] " + newFileName + ".subnet");
            env->getIPTable()->outputDictionnary("[Stopped] " + newFileName + ".ip");
        }
        
        cout << "Subnets and IP dictionnary have been saved respectively in:\n";
        cout << "-[Stopped] " << newFileName << ".subnet\n";
        cout << "-[Stopped] " << newFileName << ".ip" << endl;
        
        if(g != NULL)
            delete g;
        
        if(cuckoo != NULL)
            delete cuckoo;
        
        if(result != NULL)
            delete result;
        
        delete env;
        return 1;
    }
    catch(InvalidParameterException &e)
    {
        cout << "Use \"--help\" or \"-h\" parameter to reach help" << endl;
        delete env;
        return 1;
    }
    
    delete env;
    return 0;
}
#include "LootConfigFile.h"
#include "LootConfig.h"
#include "DefaultLootConfig.h"
#include "VanillaData.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <iostream>
#include <stdexcept>
using namespace PL;
unsigned checks=0;
void check(bool b){++checks;if(!b)throw std::runtime_error("Config file regression");}
std::string contents(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int run(int argc,char** argv){
 check(argc==2);auto dir=std::filesystem::path(argv[1])/("dpl-config-test-"+std::to_string(GetCurrentProcessId()));std::filesystem::create_directories(dir);
 auto file=dir/"DynamicPickpocketLoot.ini";
 check(ensureDefaultConfig(file,DefaultLootConfig).status==DefaultConfigStatus::Created);
 check(contents(file)==DefaultLootConfig);
 std::ifstream in(file);auto parsed=parseLootConfig(in);in.close();check(parsed.issues.empty()&&parsed.entries.size()==std::size(Vanilla::pools));
 for(unsigned i=0;i<parsed.entries.size();++i){const auto& a=parsed.entries[i];const auto& b=Vanilla::pools[i];check(a.localID==b.form&&a.category==b.category&&a.minimum==b.minimum&&a.childSafe==b.childSafe&&a.prisonerSafe==b.prisonerSafe);}
 {std::ofstream f(file);f<<"[Custom]\nuser data\n";}auto custom=contents(file);
 check(ensureDefaultConfig(file,DefaultLootConfig).status==DefaultConfigStatus::Existing&&contents(file)==custom);
 {std::ofstream f(file,std::ios::trunc);}check(ensureDefaultConfig(file,DefaultLootConfig).status==DefaultConfigStatus::Existing&&contents(file).empty());
 check(SetFileAttributesW(file.c_str(),FILE_ATTRIBUTE_READONLY));check(ensureDefaultConfig(file,DefaultLootConfig).status==DefaultConfigStatus::Existing&&contents(file).empty());check(SetFileAttributesW(file.c_str(),FILE_ATTRIBUTE_NORMAL));
 auto race=dir/"race.ini";DefaultConfigResult a{},b{};
 std::thread one([&]{a=ensureDefaultConfig(race,"complete first default");}),two([&]{b=ensureDefaultConfig(race,"complete second default");});one.join();two.join();
 check((a.status==DefaultConfigStatus::Created)+(b.status==DefaultConfigStatus::Created)==1);
 check((a.status==DefaultConfigStatus::Existing)+(b.status==DefaultConfigStatus::Existing)==1);
 check(contents(race)=="complete first default"||contents(race)=="complete second default");
 auto folder=dir/"folder.ini";std::filesystem::create_directory(folder);check(ensureDefaultConfig(folder,DefaultLootConfig).status==DefaultConfigStatus::Failed&&std::filesystem::is_directory(folder));
 auto large=dir/"large.ini";check(ensureDefaultConfig(large,std::string(1024*1024+1,'x')).status==DefaultConfigStatus::Failed&&!std::filesystem::exists(large));
 auto nested=dir/"nested"/L"config-\u00e9.ini";check(ensureDefaultConfig(nested,"full default").status==DefaultConfigStatus::Created&&contents(nested)=="full default");
 auto blocked=dir/"blocked";{std::ofstream f(blocked);f<<"keep";}check(ensureDefaultConfig(blocked/"config.ini",DefaultLootConfig).status==DefaultConfigStatus::Failed&&contents(blocked)=="keep");
 std::filesystem::remove(file);std::filesystem::remove(race);std::filesystem::remove(folder);std::filesystem::remove(nested);std::filesystem::remove(nested.parent_path());std::filesystem::remove(blocked);check(std::filesystem::is_empty(dir));std::filesystem::remove(dir);
 std::cout<<"Passed "<<checks<<" config creation/default-pool checks. Existing, empty, read-only and concurrent custom files are preserved.\n";return 0;
}
int main(int argc,char** argv){try{return run(argc,argv);}catch(const std::exception& e){std::cerr<<"After "<<checks<<": "<<e.what()<<"\n";return 1;}}

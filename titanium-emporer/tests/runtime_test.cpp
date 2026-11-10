#include "runtime.hpp"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<"\n";return EXIT_FAILURE;}}while(false)
struct Fake final:titanium::Backend{
    int enabled=0,disabled=0;bool succeeds=true;
    bool enable(titanium::Feature)override{++enabled;return succeeds;}
    void disable(titanium::Feature)noexcept override{++disabled;}
};
int main(){
    using namespace titanium;Fake b;
    {
        Runtime runtime(b);
        CHECK(!runtime.set(Feature::Fly,true));
        Evidence e{Session::Realm,false,1,{true,true,true}};
        runtime.update(e);CHECK(!runtime.set(Feature::Xray,true));
        e.verified_build=true;
        for(auto s:{Session::Unknown,Session::Menu,Session::DedicatedServer}){
            e.session=s;runtime.update(e);CHECK(!runtime.set(Feature::Xray,true));
        }
        CHECK(b.enabled==0);
        e.session=Session::Realm;runtime.update(e);
        CHECK(runtime.set(Feature::Xray,true));CHECK(runtime.set(Feature::Xray,true));CHECK(b.enabled==1);
        e.session=Session::DedicatedServer;runtime.update(e);CHECK(b.disabled==1);
        e.session=Session::LocalWorld;e.supported[2]=false;runtime.update(e);CHECK(!runtime.set(Feature::Fly,true));
        b.succeeds=false;CHECK(!runtime.set(Feature::NoFall,true));b.succeeds=true;
        CHECK(runtime.set(Feature::NoFall,true));++e.generation;runtime.update(e);CHECK(b.disabled==2);
        CHECK(runtime.set(Feature::NoFall,true));e.verified_build=false;runtime.update(e);CHECK(b.disabled==3);
        e.verified_build=true;e.generation=0;runtime.update(e);CHECK(!runtime.set(Feature::Xray,true));
        e.generation=10;runtime.update(e);CHECK(runtime.set(Feature::Xray,true));
        CHECK(!runtime.set(static_cast<Feature>(100),true));
    }
    CHECK(b.disabled==4);
    std::cout<<"PASS runtime: denied sessions, build and world evidence, activation failure, transitions and cleanup\n";
}

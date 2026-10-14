#include "core/profile.hpp"
#include <cassert>
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace eoq;
void near(double a,double b,double eps=1e-10) { if(std::abs(a-b)>eps) throw std::runtime_error("Numeric mismatch"); }
void rejects(const std::string& text) {
    bool threw=false; try { parseProfile(text); } catch(const std::exception&) { threw=true; }
    if(!threw) throw std::runtime_error("Invalid profile accepted");
}
int main(int argc,char** argv) {
    if(argc==2 && std::string(argv[1])=="--units") {
        for(double cm:{-3.518761694,0.548033649,-3.850047290,15.500000119,
                       -2.800574,0.1952004,-2.923867,15.46455,.01,1.,100.,9999.9})
            std::cout<<std::setprecision(17)<<cm<<','<<std::bit_cast<unsigned>(profileCentimetresToMetres(cm))<<'\n';
        return 0;
    }
    if(argc==2 && std::string(argv[1])=="--sweep") {
        std::cout<<std::setprecision(17);
        for(int i=0;i<2000;i++) {
            Vec re{double(i%179-89),double(i*7%359-179),double(i*11%359-179)};
            Vec oe{double(i*3%179-89),double(i*5%359-179),double(i*13%359-179)};
            Vec ae{double(i*17%179-89),double(i*19%359-179),double(i*23%359-179)};
            Pose raw{{.1,.9,-.4},unityEuler(re)}, off=profileOffset({-.02,.006,-.04},unityEuler(oe),.155);
            Pose anchor{{-.005,.03,.055},unityEuler(ae)};
            auto final=controllerPose(raw,metaLinkBasis(),off,anchor);
            for(auto v:{re.x,re.y,re.z,oe.x,oe.y,oe.z,ae.x,ae.y,ae.z,
                         final.position.x,final.position.y,final.position.z,
                         final.rotation.x,final.rotation.y,final.rotation.z,final.rotation.w}) std::cout<<v<<',';
            auto e=unityEuler(final.rotation); std::cout<<e.x<<','<<e.y<<','<<e.z<<'\n';
        }
        return 0;
    }
    if(argc==2) {
        std::ifstream f(argv[1]); std::ostringstream s; s<<f.rdbuf();
        auto p=parseProfile(s.str()); std::cout<<serializeProfile(p); return 0;
    }
    Profile p; p.unixTimestamp=1789340527; p.left.pivot={-.0351876187,.00548033655,-.0385004735};
    auto basis=metaLinkBasis();
    assert(std::bit_cast<unsigned>(float(basis.position.y))==0xbcf5c28f);
    assert(std::bit_cast<unsigned>(float(basis.position.z))==0xbd23d70a);
    assert(std::bit_cast<unsigned>(float(basis.rotation.x))==0xbf000000);
    assert(std::bit_cast<unsigned>(float(basis.rotation.w))==0x3f5db3d7);
    p.left.euler={8.940301,25.1463318,41.2388077}; p.right.euler={8.9403,-25.1463318,-41.2388};
    p.left.hasReference=true; p.left.reference={.1,.2,.3,.9};
    auto text=serializeProfile(p);
    auto copy=parseProfile(text);
    near(copy.left.pivot.x,p.left.pivot.x); near(copy.left.zOffset,.155);
    near(copy.left.reference.w,.9); assert(copy.left.hasReference && copy.unixTimestamp==p.unixTimestamp);
    near(copy.left.offset().rotation.x,.1476244372,1e-9);
    near(copy.left.offset().position.x,.0298765743,1e-9);
    rapidjson::Document doc; doc.Parse(text.c_str());
    doc["Version"].SetString("1.0",doc.GetAllocator());
    auto dump=[&] { rapidjson::StringBuffer buf; rapidjson::Writer<rapidjson::StringBuffer> w(buf); doc.Accept(w);return std::string(buf.GetString()); };
    rejects(dump()); doc["Version"].SetString("2.0",doc.GetAllocator());
    doc["RightSaberRotationEuler"]["x"].SetString("oops",doc.GetAllocator()); rejects(dump());
    doc.Parse(text.c_str()); doc.RemoveMember("RightSaberPivotPosition"); rejects(dump());
    doc.Parse(text.c_str()); for(auto key:{"x","y","z","w"}) doc["LeftSaberReference"][key].SetDouble(0); rejects(dump());
    doc.Parse(text.c_str()); doc["LeftSaberHasReference"].SetInt(1); rejects(dump());
    rejects("{truncated"); rejects("[]");
    for(auto angles:{Vec{90,22,-31},Vec{-90,14,73},Vec{14.6,13.7,38.9}}) {
        auto q=unityEuler(angles),round=unityEuler(unityEuler(q));
        near(std::abs(q.x*round.x+q.y*round.y+q.z*round.z+q.w*round.w),1.);
    }
    std::cout<<"Profile units, schema validation, reference preservation and Euler singularity tests passed.\n";
}

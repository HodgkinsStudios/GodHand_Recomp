#include "iop_compat_test_support.h"
#include "emulator/core/iop_cpu.h"
#include "emulator/core/iop_kernel.h"
#include "emulator/core/iop_memory.h"
#include "emulator/imports/iop_cdvd.h"
#include "emulator/services/iop_module_loader.h"
#include <array>
#include <string>
using namespace iop_test;
namespace {
constexpr uint32_t SID_SEARCH = 0x80000597u;
void le32(std::vector<uint8_t>& b, size_t p, uint32_t v) {
    for (unsigned i=0; i<4u; ++i) b[p+i]=static_cast<uint8_t>(v>>(8u*i));
}
void rec(std::vector<uint8_t>& b,size_t at,uint32_t lsn,uint32_t size,
         const std::string &name, bool directory) {
    const size_t len=33u+name.size()+((33u+name.size())&1u);
    b[at]=static_cast<uint8_t>(len);
    le32(b,at+2u,lsn);
    le32(b,at+10u,size);
    b[at+25u]=directory ? 2u : 0u;
    b[at+32u]=static_cast<uint8_t>(name.size());
    std::copy(name.begin(),name.end(),b.begin()+static_cast<std::ptrdiff_t>(at+33u));
}
void iso(Host& h) {
    h.file.assign(23u*2048u,0u);
    h.isoImagePath="in-memory.iso";
    auto& b=h.file;
    b[16u*2048u]=1u;
    std::memcpy(b.data()+16u*2048u+1u,"CD001",5u);
    b[16u*2048u+6u]=1u;
    rec(b,16u*2048u+156u,20u,2048u,std::string(1, '\0'),true);
    rec(b,20u*2048u,21u,123u,"README.TXT;1",false);
}
void dormant() {
    Host host;
    IopSubsystem iop(host);
    require(!iop.canBindRpc(SID_SEARCH),"CDVDFSV must remain dormant before boot");
    require(iop.loadModule("rom0:CDVDFSV").moduleId>0,"ROM0 driver load failed");
    require(iop.canBindRpc(SID_SEARCH),"ROM0 driver did not register SearchFile");
    iop.reset();
    require(!iop.canBindRpc(SID_SEARCH),"reset retained CDVD server");
}
void searchRealIso() {
    Host host;
    iso(host);
    IopSubsystem iop(host);
    require(iop.loadModule("rom0:CDVDFSV").moduleId>0,"load failed");
    std::array<uint8_t,292> p{};
    const char* path="cdrom0:\\README.TXT;1";
    std::memcpy(p.data()+32u,path,std::strlen(path)+1u);
    const uint32_t addr=0x600u;
    std::memcpy(p.data()+288u,&addr,4u);
    require(host.writeGuest(addr,p.data(),p.size()),"packet failed");
    auto q=request(SID_SEARCH,0u,4u);
    q.send={addr,static_cast<uint32_t>(p.size())};
    require(iop.handleRpc(q).handled,"RPC not handled");
    require(host.word(0x800u)==1u,"ISO file not found");
    require(host.word(addr)==21u && host.word(addr+4u)==123u,"original LSN or size lost");
    require(std::memcmp(host.guest.data()+addr+8u,"README.TXT",10u)==0,"bad file name");
}
void missingIsNotFound() {
    Host host;
    iso(host);
    IopSubsystem iop(host);
    require(iop.loadModule("rom0:CDVDFSV").moduleId>0,"load failed");
    std::array<uint8_t,292> p{};
    const char* path="cdrom0:\\NOT_THERE.BIN;1";
    std::memcpy(p.data()+32u,path,std::strlen(path)+1u);
    require(host.writeGuest(0x600u,p.data(),p.size()),"packet failed");
    auto q=request(SID_SEARCH,0u,4u);
    q.send={0x600u,292u};
    require(iop.handleRpc(q).handled && host.word(0x800u)==0u,"missing file fabricated");
    host.isoImagePath.clear();
    require(iop.handleRpc(q).handled && host.word(0x800u)==0u,"missing media fabricated");
}
void nativeCdvdImportUsesOriginalLsn() {
    Host host;
    iso(host);
    ps2x::iop::detail::IopMemory memory;
    ps2x::iop::detail::IopKernel kernel(memory);
    kernel.reset();
    ps2x::iop::detail::IopCdvd cdvd(host, memory, kernel);
    cdvd.reset();
    const char path[] = "cdrom0:\\README.TXT;1";
    require(memory.writeRam(0x1800u, path, sizeof(path)), "IOP filename not writable");
    ps2x::iop::detail::IopCpuState cpu{};
    cpu.gpr[4] = 0x2000u;
    cpu.gpr[5] = 0x1800u;
    require(cdvd.dispatchImport(10u, cpu) && cpu.gpr[2] == 1u, "physical cdvdman file search failed");
    require(memory.read32(0x2000u) == 21u && memory.read32(0x2004u) == 123u,
            "physical cdvdman returned a synthetic LSN incompatible with original ISO sectors");
}
void readDiscIrxDirectly() {
    Host host;
    iso(host);
    for (uint32_t i = 0u; i < 123u; ++i)
        host.file[21u*2048u+i] = static_cast<uint8_t>(i ^ 0x5Au);
    std::vector<uint8_t> contents;
    require(ps2x::iop::detail::IopModuleLoader::readWholeHostFile(
                host, "cdrom0:\\README.TXT;1", contents), "ISO-only module not opened");
    require(contents.size() == 123u, "incorrect ISO module byte length");
    for (uint32_t i = 0u; i < 123u; ++i)
        require(contents[i] == static_cast<uint8_t>(i ^ 0x5Au), "IRX bytes did not match disc");
    require(!ps2x::iop::detail::IopModuleLoader::readWholeHostFile(
                host, "cdrom0:\\MISSING.IRX;1", contents), "nonexistent module opened");
    require(contents.empty(), "a failed module read returned stale file bytes");
    require(!ps2x::iop::detail::IopModuleLoader::readWholeHostFile(
                host, "host0:README.TXT", contents), "host file missing was substituted with ISO data");
    host.file.resize(21u*2048u+50u);
    require(!ps2x::iop::detail::IopModuleLoader::readWholeHostFile(
                host, "cdrom0:\\README.TXT;1", contents), "truncated disc extent read accepted");
}
void bounds() {
    Host host;
    iso(host);
    IopSubsystem iop(host);
    require(iop.loadModule("rom0:CDVDFSV").moduleId>0,"load failed");
    auto q=request(SID_SEARCH,0u,4u);
    q.send={0x600u,64u};
    require(iop.handleRpc(q).handled && host.word(0x800u)==0u,"truncated packet accepted");
}
}
int main() {
    const Test tests[] = {
        {"CDVDFSV ROM0 module lifecycle",dormant},
        {"SearchFile returns original ISO9660 sector and length",searchRealIso},
        {"Absent media and missing files return failure",missingIsNotFound},
        {"Short SearchFile requests are rejected",bounds},
        {"IOP cdvdman SearchFile uses physical ISO LSN",nativeCdvdImportUsesOriginalLsn},
        {"IOP IRX loader reads original ISO bytes and rejects missing/truncated modules",readDiscIrxDirectly},
    };
    return run(tests);
}

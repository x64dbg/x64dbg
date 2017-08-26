#pragma once

#include "_global.h"
#include "addrinfo.h"
#include <algorithm>

struct PeData
{
    enum Error
    {
        ErrorUnknown,
        ErrorCreateFile,
        ErrorCreateFileMapping,
        ErrorMapViewOfFile,
        ErrorException,
        ErrorDosSignature,
        ErrorNtSignature,
        ErrorMachine,
        ErrorOptionalHeaderMagic,
        ErrorSuccess
    } error = ErrorUnknown;
    DWORD LastError = ERROR_SUCCESS;

    static const char* errorName(Error error)
    {
#define casename(x) case x: return #x
        switch(error)
        {
            casename(ErrorUnknown);
            casename(ErrorCreateFile);
            casename(ErrorCreateFileMapping);
            casename(ErrorMapViewOfFile);
            casename(ErrorException);
            casename(ErrorDosSignature);
            casename(ErrorNtSignature);
            casename(ErrorMachine);
            casename(ErrorSuccess);
        default:
            return "<default>";
        }
#undef casename
    }

    bool IsDll = false;
};

bool ParsePe(const wchar_t* szFileName, PeData & data)
{
    data = PeData();
    auto hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        // SEC_IMAGE will load the file like the Windows loader would, saving us RVA -> File offset conversion crap.
        // NOTE: this WILL rebase the file so it should be used with caution.
        auto hMappedFile = CreateFileMappingW(hFile, nullptr, PAGE_READONLY | SEC_IMAGE, 0, 0, nullptr);
        if(hMappedFile)
        {
            auto fileMap = MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0);
            if(fileMap)
            {
                __try
                {
                    auto pidh = PIMAGE_DOS_HEADER(fileMap);
                    if(pidh->e_magic == IMAGE_DOS_SIGNATURE)
                    {
                        auto pnth = PIMAGE_NT_HEADERS(ULONG_PTR(fileMap) + pidh->e_lfanew);
                        if(pnth->Signature == IMAGE_NT_SIGNATURE)
                        {
                            auto Machine = pnth->FileHeader.Machine;
                            if(Machine == ArchValue(IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_AMD64))
                            {
                                if(pnth->OptionalHeader.Magic == ArchValue(IMAGE_NT_OPTIONAL_HDR32_MAGIC, IMAGE_NT_OPTIONAL_HDR64_MAGIC))
                                {
                                    // NOTE: try to access fields in the order they appear in memory (just in case the header is cut off)
                                    data.IsDll = (pnth->FileHeader.Characteristics & IMAGE_FILE_DLL) == IMAGE_FILE_DLL;
                                    data.error = PeData::ErrorSuccess;
                                }
                                else
                                    data.error = PeData::ErrorOptionalHeaderMagic;
                            }
                            else
                                data.error = PeData::ErrorMachine;
                        }
                        else
                            data.error = PeData::ErrorNtSignature;
                    }
                    else
                        data.error = PeData::ErrorDosSignature;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    data.error = PeData::ErrorException;
                }
                UnmapViewOfFile(fileMap);
            }
            else
            {
                data.LastError = GetLastError();
                data.error = PeData::ErrorMapViewOfFile;
            }
            CloseHandle(hMappedFile);
        }
        else
        {
            data.LastError = GetLastError();
            data.error = PeData::ErrorCreateFileMapping;
        }
        CloseHandle(hFile);
    }
    else
    {
        data.LastError = GetLastError();
        data.error = PeData::ErrorCreateFile;
    }
    return false;
}

class PeSectionMap
{
    using RangeSectionMap = std::map<Range, size_t, RangeCompare>;
    RangeSectionMap mOffsetSectionMap;
    RangeSectionMap mRvaSectionMap;
    std::vector<IMAGE_SECTION_HEADER> mSections;
    uint32 mAlignment;
    const int HeaderSection = -1;

    template<typename T>
    static T alignAdjustSize(T size, uint32 alignment) //TODO: check this
    {
        return size + (alignment - 1) & ~(alignment - 1);
    }

    template<typename T>
    static T alignAdjustAddress(T address, uint32 alignment) //TODO: check this
    {
        return address & ~(alignment - 1);
    }

public:
    int ParseSections(std::vector<IMAGE_SECTION_HEADER> & sections, uint32 sectionAlignment, uint32 fileAlignment)
    {
        mSections = sections;

        //create rva/offset -> section maps
        auto minSection = std::min_element(mSections.begin(), mSections.end(), [](const IMAGE_SECTION_HEADER & a, const IMAGE_SECTION_HEADER & b)
        {
            if(!a.SizeOfRawData)
                return false;
            if(!b.SizeOfRawData)
                return true;
            return a.PointerToRawData < b.PointerToRawData;
        });
        //insert pe header offset/rva (file start >| first (physical) section is the PE header)
        if(minSection != mSections.end() && minSection->PointerToRawData)
        {
            //TODO: rounding down with FileAlignment (duphead.exe)
            mOffsetSectionMap.insert({ Range(0, minSection->PointerToRawData - 1), HeaderSection });
            mRvaSectionMap.insert({ Range(0, minSection->PointerToRawData - 1), HeaderSection });
        }

        for(size_t i = 0; i < mSections.size(); i++)
        {
            const auto & section = mSections.at(i);
            //offset -> section index
            auto offset = section.PointerToRawData;
            //bigSoRD.exe: if raw size is bigger than virtual size, then virtual size is taken.
            auto rsize = min(section.SizeOfRawData, section.Misc.VirtualSize);
            if(!rsize) //65535sects.exe
                continue;
            auto range = Range(offset, offset + rsize - 1);
            auto result = mOffsetSectionMap.insert({ range, i });
            if(!result.second)
            {
                //this happens if two sections share raw data:
                //imports_1210.exe, secinsec.exe, maxsecXP.exe, dupsec.exe, bigSoRD.exe
                auto prange = result.first->first;
                dprintf("trying to insert Range(0x%X, 0x%X)\n", offset, offset + rsize - 1);
                dprintf("prevented by Range(0x%X, 0x%X)\n", prange.first, prange.second);


                if(range.first >= prange.first && range.second <= prange.second)
                {
                    dputs("new range is contained in the preventing range => do nothing");
                }
                else //range has to be split and the two ranges have to be tried to be added recursively
                    return 1;
            }

            //rva -> section index
            auto rva = alignAdjustAddress(section.VirtualAddress, sectionAlignment);
            if(!mRvaSectionMap.insert({ Range(rva, rva + rsize - 1), i }).second)
                return 2;
        }

        return 0;
    }

    uint32 ConvertOffsetToRva(uint32 offset)
    {
        if(!mOffsetSectionMap.size()) //TODO: verify this (no sections means direct mapping)
            return offset;
        const auto found = mOffsetSectionMap.find(Range(offset, offset));
        if(found == mOffsetSectionMap.end())
            return -1;
        auto index = found->second;
        if(index == HeaderSection)
            return offset;
        const auto & section = mSections.at(index);
        offset -= uint32(found->first.first); //adjust the offset to be relative to the offset range in the map
        return alignAdjustAddress(section.VirtualAddress, mAlignment) + offset;
    }

    uint32 ConvertRvaToOffset(uint32 rva)
    {
        if(!mRvaSectionMap.size()) //TODO: verify this (no sections means direct mapping)
            return rva;
        const auto found = mRvaSectionMap.find(Range(rva, rva));
        if(found == mRvaSectionMap.end())
            return -1;
        auto index = found->second;
        if(index == HeaderSection)
            return rva;
        const auto & section = mSections.at(index);
        rva -= uint32(found->first.first); //adjust the rva to be relative to the rva range in the map
        return section.PointerToRawData + rva;
    }
};

struct PeDataBasic
{
    enum Error
    {
        ErrorUnknown,
        ErrorException,
        ErrorDosSignature,
        ErrorNtSignature,
        ErrorMachine,
        ErrorOptionalHeaderMagic,
        ErrorSuccess
    } error = ErrorUnknown;

    static const char* errorName(Error error)
    {
#define casename(x) case x: return #x
        switch(error)
        {
            casename(ErrorUnknown);
            casename(ErrorException);
            casename(ErrorDosSignature);
            casename(ErrorNtSignature);
            casename(ErrorMachine);
            casename(ErrorSuccess);
        default:
            return "<default>";
        }
#undef casename
    }

    uint16 Characteristics = 0;
    duint EntryPoint = 0;
    duint ImageBase = 0;
    uint32 SectionAlignment = 1;
    uint32 FileAlignment = 1;
    std::vector<IMAGE_SECTION_HEADER> sections;
};

void ParsePeBasic(ULONG_PTR FileMapVA, PeDataBasic & data)
{
    __try
    {
        auto pidh = PIMAGE_DOS_HEADER(FileMapVA);
        if(pidh->e_magic == IMAGE_DOS_SIGNATURE)
        {
            auto pnth = PIMAGE_NT_HEADERS(FileMapVA + pidh->e_lfanew);
            if(pnth->Signature == IMAGE_NT_SIGNATURE)
            {
                auto Machine = pnth->FileHeader.Machine;
                if(Machine == ArchValue(IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_AMD64))
                {
                    data.Characteristics = pnth->FileHeader.Characteristics;
                    auto NumberOfSections = pnth->FileHeader.NumberOfSections;
                    if(pnth->OptionalHeader.Magic == ArchValue(IMAGE_NT_OPTIONAL_HDR32_MAGIC, IMAGE_NT_OPTIONAL_HDR64_MAGIC))
                    {
                        data.EntryPoint = pnth->OptionalHeader.AddressOfEntryPoint;
                        data.ImageBase = pnth->OptionalHeader.ImageBase;
                        data.SectionAlignment = pnth->OptionalHeader.SectionAlignment;
                        data.FileAlignment = pnth->OptionalHeader.FileAlignment;
                        data.sections.resize(NumberOfSections);
                        auto pish = IMAGE_FIRST_SECTION(pnth);
                        for(auto i = 0; i < NumberOfSections; i++)
                            memcpy(data.sections.data() + i, pish + i, sizeof(IMAGE_SECTION_HEADER));
                        data.error = PeDataBasic::ErrorSuccess;
                    }
                    else
                        data.error = PeDataBasic::ErrorOptionalHeaderMagic;
                }
                else
                    data.error = PeDataBasic::ErrorMachine;
            }
            else
                data.error = PeDataBasic::ErrorNtSignature;
        }
        else
            data.error = PeDataBasic::ErrorDosSignature;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        data.error = PeDataBasic::ErrorException;
    }
}

void testFileBasic(const std::wstring & file)
{
    dprintf("Testing (basic) %S ...\n", file.c_str());
    auto hFile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        auto hMappedFile = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if(hMappedFile)
        {
            auto fileMap = MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0);
            if(fileMap)
            {
                PeDataBasic basic;
                ParsePeBasic(ULONG_PTR(fileMap), basic);
                if(basic.error != PeDataBasic::ErrorSuccess)
                    dprintf("[!!!] ");
                else
                    dprintf("      ");
                dprintf("error: %s\n", basic.errorName(basic.error));
                PeSectionMap sectionMap;
                auto secErr = sectionMap.ParseSections(basic.sections, basic.SectionAlignment, basic.FileAlignment);
                if(secErr)
                    dprintf("[!!!] ParseSections failed (%d)...\n", secErr);
                UnmapViewOfFile(fileMap);
            }
            else
                dputs("MapViewOfFile");
            CloseHandle(hMappedFile);
        }
        else
            dputs("CreateFileMapping");
        CloseHandle(hFile);
    }
    else
        dputs("CreateFile");
}

void testFile(const std::wstring & file)
{
    dprintf("Testing (advanced) %S ...\n", file.c_str());

    PeData data;
    ParsePe(file.c_str(), data);
    if(data.error != PeData::ErrorSuccess)
        dprintf("[!!!] ");
    else
        dprintf("      ");
    dprintf("error: %s (LastError: %s)\n", data.errorName(data.error), ErrorCodeToName(data.LastError).c_str());
}

const wchar_t* peTestFiles[] =
{
    L"65535sects.exe",
    L"96emptysections.exe",
    L"96workingsections.exe",
    L"appendeddata.exe",
    L"appendedhdr.exe",
    L"appendedsecttbl.exe",
    L"apphdrW7.exe",
    L"appsectableW7.exe",
    L"aslr-ld.exe",
    L"aslr.dll",
    L"bigalign.exe",
    L"bigib.exe",
    L"bigsec.exe",
    L"bigSoRD.exe",
    L"bottomsecttbl.exe",
    L"cfgbogus.exe",
    L"compiled.exe",
    L"copyright.exe",
    L"ctxt-ld.exe",
    L"ctxt.dll",
    L"debug.exe",
    L"delaycorrupt.exe",
    L"delayfake.exe",
    L"delayimports.exe",
    L"dep.exe",
    L"dll-dynld.exe",
    L"dll-dynunicld.exe",
    L"dll-ld.exe",
    L"dll-webdavld.exe",
    L"dll.dll",
    L"dllbound-ld.exe",
    L"dllbound-redirld.exe",
    L"dllbound-redirldXP.exe",
    L"dllbound.dll",
    L"dllbound2.dll",
    L"dllcfgdup-dynld.exe",
    L"dllcfgdup.dll",
    L"dllemptyexp-ld.exe",
    L"dllemptyexp.dll",
    L"dllextep-ld.exe",
    L"dllextep.dll",
    L"dllfakess-dynld.exe",
    L"dllfakess-ld.exe",
    L"dllfakess.dll",
    L"dllfw-ld.exe",
    L"dllfw.dll",
    L"dllfwloop-ld.exe",
    L"dllfwloop.dll",
    L"dllmaxvals-dynld.exe",
    L"dllmaxvals-ld.exe",
    L"dllmaxvals.dll",
    L"dllnegep-ld.exe",
    L"dllnegep.dll",
    L"dllnoexp-dynld.exe",
    L"dllnoexp.dll",
    L"dllnomain-ld.exe",
    L"dllnomain.dll",
    L"dllnomain2-dynld.exe",
    L"dllnomain2.dll",
    L"dllnoreloc-ld.exe",
    L"dllnoreloc.dll",
    L"dllnullep-dynld.exe",
    L"dllnullep-ld.exe",
    L"dllnullep.dll",
    L"dllord-ld.exe",
    L"dllord.dll",
    L"dllweirdexp-ld.exe",
    L"dllweirdexp.dll",
    L"dosZMXP.exe",
    L"dotnet20.exe",
    L"driver.sys",
    L"dump_imports.exe",
    L"duphead.exe",
    L"dupsec.exe",
    L"d_nonnull-ld.exe",
    L"d_nonnull.dll",
    L"d_resource-ld.exe",
    L"d_resource.dll",
    L"d_tiny-ld.exe",
    L"d_tiny.dll",
    L"exceptions.exe",
    L"exe2pe.exe",
    L"exportobf.exe",
    L"exportsdata.exe",
    L"exports_doc.exe",
    L"exports_order.exe",
    L"fakenet.exe",
    L"fakeregs.exe",
    L"fakeregslib.dll",
    L"fakerelocs.exe",
    L"foldedhdr.exe",
    L"foldedhdrW7.exe",
    L"footer.exe",
    L"gui.exe",
    L"hard_imports.exe",
    L"hdrcode.exe",
    L"hdrdata.exe",
    L"hiddenappdata1.exe",
    L"hiddenappdata2.exe",
    L"ibkernel.exe",
    L"ibkmanual.exe",
    L"ibknoreloc64.exe",
    L"ibnullXP.exe",
    L"ibreloc.exe",
    L"ibrelocW7.exe",
    L"impbyord.exe",
    L"imports.exe",
    L"importsdotXP.exe",
    L"importshint.exe",
    L"imports_apimsW7.exe",
    L"imports_badterm.exe",
    L"imports_bogusIAT.exe",
    L"imports_corruptedIAT.exe",
    L"imports_iatindesc.exe",
    L"imports_mixed.exe",
    L"imports_multidesc.exe",
    L"imports_nnIAT.exe",
    L"imports_noext.exe",
    L"imports_noint.exe",
    L"imports_nothunk.exe",
    L"imports_relocW7.exe",
    L"imports_tinyW7.exe",
    L"imports_tinyXP.exe",
    L"imports_virtdesc.exe",
    L"imports_vterm.exe",
    L"ldrsnaps.exe",
    L"ldrsnaps64.exe",
    L"lfanew_relocW7.exe",
    L"lfanew_relocXP.exe",
    L"lowsubsys.exe",
    L"manifest.exe",
    L"manifest_broken.exe",
    L"manifest_bsod.exe",
    L"manyimportsW7.exe",
    L"maxsecW7.exe",
    L"maxsecXP.exe",
    L"maxsec_lowaligW7.exe",
    L"maxvals.exe",
    L"memshared-ld.exe",
    L"memshared.dll",
    L"mini.exe",
    L"mscoree.exe",
    L"multiss.exe",
    L"multiss_con.exe",
    L"multiss_drv.sys",
    L"multiss_gui.exe",
    L"mz.exe",
    L"namedresource.exe",
    L"no0code.exe",
    L"normal.exe",
    L"normal64.exe",
    L"nosectionW7.exe",
    L"nosectionXP.exe",
    L"nothing-ld.exe",
    L"nothing.dll",
    L"no_dd.exe",
    L"no_dd64.exe",
    L"no_dep.exe",
    L"no_seh.exe",
    L"nullEP.exe",
    L"nullSOH-XP.exe",
    L"nullvirt.exe",
    L"ownexports.exe",
    L"ownexports2.exe",
    L"ownexportsdot.exe",
    L"pdf.exe",
    L"pdf_zip_pe.exe",
    L"quine.exe",
    L"reloc4.exe",
    L"reloc9.exe",
    L"reloccrypt.exe",
    L"reloccryptW8.exe",
    L"reloccryptXP.exe",
    L"relocsstripped.exe",
    L"relocsstripped64.exe",
    L"reshdr.exe",
    L"resource.exe",
    L"resource2.exe",
    L"resourceloop.exe",
    L"resource_icon.exe",
    L"resource_string.exe",
    L"safeseh.exe",
    L"safeseh_fly.exe",
    L"sc.exe",
    L"secinsec.exe",
    L"seh_change64.exe",
    L"shuffledsect.exe",
    L"signature.exe",
    L"skippeddynbase.exe",
    L"slackspace.exe",
    L"ss63.exe",
    L"ss63nocookie.exe",
    L"standard.exe",
    L"tiny.exe",
    L"tinydll-ld.exe",
    L"tinydll.dll",
    L"tinydllXP-ld.exe",
    L"tinydllXP.dll",
    L"tinydrivXP.sys",
    L"tinygui.exe",
    L"tinynet.exe",
    L"tinyW7.exe",
    L"tinyW7x64.exe",
    L"tinyW7_3264.exe",
    L"tinyXP.exe",
    L"tls.exe",
    L"tls64.exe",
    L"tls_aoi.exe",
    L"tls_aoiOSDET.exe",
    L"tls_exiting.exe",
    L"tls_import.exe",
    L"tls_k32.exe",
    L"tls_noEP.exe",
    L"tls_obfuscation.exe",
    L"tls_onthefly.exe",
    L"tls_reloc.exe",
    L"tls_virtEP.exe",
    L"truncatedlast.exe",
    L"truncsectbl.exe",
    L"version_cust.exe",
    L"version_mini.exe",
    L"version_std.exe",
    L"virtEP.exe",
    L"virtgap.exe",
    L"virtrelocXP.exe",
    L"virtsectblXP.exe",
    L"weirdsord.exe",
    L"winver.exe",
    L"a.exe",
    L"a2.exe",
    L"b.exe",
    L"badlogger.exe",
    L"base.exe",
    L"base_FSG.exe",
    L"c.exe",
    L"ch22.exe",
    L"cmd_adf_sample0.exe",
    L"CoST.exe",
    L"crackme0x00.exe",
    L"crackme0x01.exe",
    L"crackme0x02.exe",
    L"crackme0x03.exe",
    L"crackme0x04.exe",
    L"crackme0x05.exe",
    L"crackme0x06.exe",
    L"crackme0x07.exe",
    L"crackme0x08.exe",
    L"crackme0x09.exe",
    L"ddsect.exe",
    L"GleeBugPeEnum.exe",
    L"hello-mingw32.exe",
    L"hellocxx-mingw32.exe",
    L"imports_1210.exe",
    L"jman.exe",
    L"Lab05-01.dll",
    L"mitigation_dynamic.exe",
    L"mitigation_nothing.exe",
    L"pe.exe",
    L"relocOSdet.exe",
    L"single_import.exe",
    L"test.exe",
    L"torivahti.exe",
    L"vista-glass.exe"
};

bool cbPeTest(int argc, char* argv[])
{
    for(auto i = 0; i < _countof(peTestFiles); i++)
    {
        auto file = std::wstring(L"C:\\!exclude\\pe\\bin\\") + peTestFiles[i];
        testFileBasic(file);
        testFile(file);
        dputs("");
    }
    return false;
}
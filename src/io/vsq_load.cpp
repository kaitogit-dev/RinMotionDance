#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>

#include "rinmotiondance/model.hpp"
#include "rinmotiondance/mmd_app.hpp"
#include "rinmotiondance/ported_funcs.hpp"
#include "rinmotiondance/panel_controls.hpp"

namespace rinmotiondance {
namespace {

const char kJpOpenFailFmt[] =
    "\x83\x74\x83\x40\x83\x43\x83\x8b\x82\xaa\x93\xc7\x82\xdd\x8d\x9e"
    "\x82\xdf\x82\xdc\x82\xb9\x82\xf1\x3a\x25\x64";
const char kJpSysEx[] =
    "\x53\x79\x73\x45\x78\x83\x43\x83\x78\x83\x93\x83\x67\x82\xaa\x82"
    "\xa0\x82\xe8\x82\xdc\x82\xb7\x81\x42\x82\xa8\x8e\xe8\x8f\xe3\x82"
    "\xb0\x81\x5f\x28\x5e\x6f\x5e\x29\x81\x5f";
const char kJpVsqCaption[] = "\x76\x73\x71\x89\xf0\x90\xcd";
const char kJpAnalysisErr[] =
    "\x76\x73\x71\x89\xf0\x90\xcd\x83\x47\x83\x89\x81\x5b\x94\xad\x90"
    "\xb6\x81\x49\x81\x49";
const char kJpNoA[] =
    "\x82\xb1\x82\xcc\x83\x82\x83\x66\x83\x8b\x82\xcc\x95\x5c\x8f\xee"
    "\x82\xc9\x81\x68\x82\xa0\x81\x68\x82\xcc\x8c\x60\x82\xaa\x82\xc8"
    "\x82\xa2\x88\xd7\x0a\x8e\xa9\x93\xae\x83\x8a\x83\x62\x83\x76\x83"
    "\x56\x83\x93\x83\x4e\x82\xcd\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82"
    "\xf1";
const char kJpNoI[] =
    "\x82\xb1\x82\xcc\x83\x82\x83\x66\x83\x8b\x82\xcc\x95\x5c\x8f\xee"
    "\x82\xc9\x81\x68\x82\xa2\x81\x68\x82\xcc\x8c\x60\x82\xaa\x82\xc8"
    "\x82\xa2\x88\xd7\x0a\x8e\xa9\x93\xae\x83\x8a\x83\x62\x83\x76\x83"
    "\x56\x83\x93\x83\x4e\x82\xcd\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82"
    "\xf1";
const char kJpNoU[] =
    "\x82\xb1\x82\xcc\x83\x82\x83\x66\x83\x8b\x82\xcc\x95\x5c\x8f\xee"
    "\x82\xc9\x81\x68\x82\xa4\x81\x68\x82\xcc\x8c\x60\x82\xaa\x82\xc8"
    "\x82\xa2\x88\xd7\x0a\x8e\xa9\x93\xae\x83\x8a\x83\x62\x83\x76\x83"
    "\x56\x83\x93\x83\x4e\x82\xcd\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82"
    "\xf1";
const char kJpNoO[] =
    "\x82\xb1\x82\xcc\x83\x82\x83\x66\x83\x8b\x82\xcc\x95\x5c\x8f\xee"
    "\x82\xc9\x81\x68\x82\xa8\x81\x68\x82\xcc\x8c\x60\x82\xaa\x82\xc8"
    "\x82\xa2\x88\xd7\x0a\x8e\xa9\x93\xae\x83\x8a\x83\x62\x83\x76\x83"
    "\x56\x83\x93\x83\x4e\x82\xcd\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82"
    "\xf1";
const char kJpTimeSig[] =
    "\x94\x8f\x8e\x71\x3d\x25\x64\x2f\x25\x64\x20\x83\x81\x83\x67\x83"
    "\x8d\x83\x6e\x81\x5b\x83\x80\x8a\xd4\x8a\x75\x3d\x25\x64\x20\x8e"
    "\x6c\x95\xaa\x89\xb9\x95\x84\x82\xa0\x82\xbd\x82\xe8\x82\xcc\x8e"
    "\x4f\x8f\x5c\x93\xf1\x95\xaa\x89\xb9\x95\x84\x82\xcc\x90\x94\x3d"
    "\x25\x64";
const char kJpKeyMajor[] =
    "\x83\x4c\x81\x5b\x28\x92\xb2\x29\x20\x81\x94\x28\x81\xf3\x29\x82"
    "\xcc\x90\x94\x3a\x25\x64\x20\x92\xb7\x92\xb2";
const char kJpKeyMinor[] =
    "\x83\x4c\x81\x5b\x28\x92\xb2\x29\x20\x81\x94\x28\x81\xf3\x29\x82"
    "\xcc\x90\x94\x3a\x25\x64\x20\x92\x5a\x92\xb2";

const char kFaceA[] = "\x82\xa0";  
const char kFaceI[] = "\x82\xa2";  
const char kFaceU[] = "\x82\xa4";  
const char kFaceO[] = "\x82\xa8";  

struct TempoEntry {
    int time;
    int tempo;
};

struct VsqEvent {           
    char consonant[6];      
    char vowel;             
    char token[9];          
    float start;            
    float end;              
    char handle[10];        
    char tail[2];           
};
static_assert(sizeof(VsqEvent) == 0x24, "event record stride");

void VsqBox(MMDApp* app, const char* enText, const char* jpText,
            const char* enCaption, const char* jpCaption) {
    MessageBoxA(static_cast<HWND>(app->Hwnd()),
                app->EnglishUI() != 0 ? enText : jpText,
                app->EnglishUI() != 0 ? enCaption : jpCaption, MB_OK);
}

void VsqAnalysisError(MMDApp* app, FILE* txt, int fd) {
    VsqBox(app,
           "vsq analysis error!!",                 
           kJpAnalysisErr,                         
           "vsq analysis",                         
           kJpVsqCaption);                         
    fclose(txt);                                   
    _close(fd);                                    
}

unsigned char* SlotModel(MMDApp* app) {
    return app->SelectedModel();
}

void VsqMorphKey(unsigned char* model, mdl::MorphRecord* morphs, int idx,
                 int frame, float weight) {
    morphs[idx].value = weight;
    RegisterMorphKeyCurrent(model, idx, frame);
}

}  

void LoadVsqFile(MMDApp* app, const wchar_t* path) {
    auto& s = *app;
    HWND hwnd = static_cast<HWND>(s.Hwnd());                       
    char line[0x100];        
    char idbuf[0x100];       
    char buf[1000];          
    char scratch[1000];      

    int fd = -1;
    const errno_t openErr =
        _wsopen_s(&fd, path, 0x8000  , 0x40  ,
                  0x80  );
    if (openErr != 0) {
        if (s.EnglishUI() != 0)
            sprintf_s(buf, 1000, "Cannot open file:%d", openErr);  
        else
            sprintf_s(buf, 1000, kJpOpenFailFmt, openErr);         
        MessageBoxA(hwnd, buf, "", 0);                             
        return;
    }

    wchar_t* ext = const_cast<wchar_t*>(wcsstr(path, L".vsq")) + 1;
    ext[0] = L't';                                                 
    ext[1] = L'x';                                                 
    ext[2] = L't';                                                 

    FILE* txt = nullptr;
    _wfopen_s(&txt, path, L"wt");                                  

    int v = 0;
    int ntracks = 0;
    int division = 0;
    ReadFixedString(fd, scratch, 4);              
    ReadBeWord(fd, &v, 4);                   
    ReadBeWord(fd, &v, 2);                   
    ReadBeWord(fd, &ntracks, 2);
    sprintf_s(buf, 1000, "[number of Track:%d]", ntracks);         
    fprintf(txt, "%s\n", buf);                                     
    ReadBeWord(fd, &division, 2);
    sprintf_s(buf, 1000, "[UnitTime:%d]", division);               
    fprintf(txt, "%s\n\n", buf);                                   

    int tempoCount = 0;
    int trackIdx = 0;
    if (ntracks > 0) {
        do {
            sprintf_s(buf, 1000, "[Track%d]", trackIdx);           
            fprintf(txt, "%s\n", buf);
            ReadFixedString(fd, scratch, 4);      
            ReadBeWord(fd, &v, 4);           

            int time = 0;                   
            for (;;) {                      
                ReadBeWord(fd, &v, 1);
                int delta = v;
                if (delta > 0x7f) {
                    do {
                        ReadBeWord(fd, &v, 1);
                        delta = ((delta - 0x80) << 7) + v;          
                    } while (v > 0x7f);
                }
                time += delta;
                int status = 0;
                ReadBeWord(fd, &status, 1);
                if (status >= 0x80 && status <= 0x8f) {
                    ReadBeWord(fd, &v, 2);
                    continue;
                }
                if (status >= 0x90 && status <= 0x9f) {
                    ReadBeWord(fd, &v, 2);
                    continue;
                }
                if (status >= 0xb0 && status <= 0xbf) {
                    ReadBeWord(fd, &v, 2);
                    continue;
                }
                if (status == 0xf0 || status == 0xf7) {             
                    MessageBoxA(hwnd, kJpSysEx, kJpVsqCaption, 0);
                    fclose(txt);
                    _close(fd);
                    return;
                }
                if (status != 0xff) {                               
                    VsqAnalysisError(app, txt, fd);
                    return;
                }

                int type = 0;
                ReadBeWord(fd, &type, 1);
                if (type == 0) {
                    ReadBeWord(fd, &v, 1);
                    continue;
                }
                if (type == 1) {                                    
                    int len = 0;
                    ReadBeWord(fd, &len, 1);
                    ReadFixedString(fd, scratch, 8);
                    ReadFixedString(fd, scratch, len - 8);
                    fprintf(txt, "%s", scratch);                    
                    continue;
                }
                if (type >= 2 && type <= 5) {                       
                    int len = 0;
                    ReadBeWord(fd, &len, 1);
                    ReadFixedString(fd, scratch, len);
                    continue;
                }
                if (type == 0x2f) {                                 
                    int len = 0;
                    ReadBeWord(fd, &len, 1);
                    if (len != 0)
                        continue;
                    sprintf_s(buf, 1000, "[End of Track%d]", trackIdx);
                    fprintf(txt, "%s\n\n", buf);                    
                    ++trackIdx;
                    break;          
                }
                if (type == 0x51) {                                 
                    int tempo = 0;
                    ReadBeWord(fd, &v, 1);    
                    ReadBeWord(fd, &tempo, 3);
                    sprintf_s(buf, 1000,
                              "[ChangeTempo time=%d,tempo=%d]",     
                              time, tempo);
                    fprintf(txt, "%s\n", buf);
                    ++tempoCount;
                    continue;
                }
                if (type == 0x58) {                                 
                    unsigned char lenByte = 0;                      
                    _read(fd, &lenByte, 1);
                    int nn = 0, dd = 0, cc = 0, bb = 0;
                    ReadBeWord(fd, &nn, 1);
                    ReadBeWord(fd, &dd, 1);
                    ReadBeWord(fd, &cc, 1);
                    ReadBeWord(fd, &bb, 1);
                    int den = 1;
                    for (int t = dd; t > 0; --t) den += den;        
                    sprintf_s(buf, 1000, kJpTimeSig,                
                              nn, den, cc, bb);  
                    continue;
                }
                if (type == 0x59) {                                 
                    int sf = 0, mi = 0;
                    ReadBeWord(fd, &v, 1);    
                    ReadBeWord(fd, &sf, 1);
                    ReadBeWord(fd, &mi, 1);
                    sprintf_s(buf, 1000,
                              mi == 0 ? kJpKeyMajor : kJpKeyMinor,  
                              sf);
                    fprintf(txt, "%s\n", buf);
                    continue;
                }
                VsqAnalysisError(app, txt, fd);                     
                return;
            }
        } while (trackIdx < ntracks);
    }

    fclose(txt);                                                    
    _close(fd);                                                     

    TempoEntry* tempoTable =
        static_cast<TempoEntry*>(malloc(
            static_cast<std::size_t>(tempoCount) * sizeof(TempoEntry)));
    FILE* rd = nullptr;
    _wfopen_s(&rd, path, L"r");                                     
    for (int k = 0; k < tempoCount; ++k) {                          
        fgets(line, 0x100, rd);
        if (strstr(line, "[ChangeTempo time=") == nullptr) {        
            do {
                fgets(line, 0x100, rd);
            } while (strstr(line, "[ChangeTempo time=") == nullptr);
        }
        char* p = strstr(line, "=") + 1;                            
        char* q = strstr(p, ",");                                   
        *q = 0;
        tempoTable[k].time = atoi(p);
        char* p2 = strstr(q + 1, "=") + 1;
        char* q2 = strstr(p2, "]");                                 
        *q2 = 0;
        tempoTable[k].tempo = atoi(p2);
    }

    if (strstr(line, "[EventList]") == nullptr) {                   
        do {
            fgets(line, 0x100, rd);
        } while (strstr(line, "[EventList]") == nullptr);
    }
    int count = 0;
    fgets(line, 0x100, rd);
    fgets(line, 0x100, rd);
    if (strstr(line, "=ID#") != nullptr) {                          
        do {
            ++count;
            fgets(line, 0x100, rd);
        } while (strstr(line, "=ID#") != nullptr);
    }
    fclose(rd);

    VsqEvent* events = static_cast<VsqEvent*>(
        malloc(static_cast<std::size_t>(count) * 0x24));
    memset(events, 0, static_cast<std::size_t>(count) * 0x24);      

    FILE* rd2 = nullptr;
    _wfopen_s(&rd2, path, L"r");
    if (strstr(line, "[EventList]") == nullptr) {
        do {
            fgets(line, 0x100, rd2);
        } while (strstr(line, "[EventList]") == nullptr);
    }
    fgets(line, 0x100, rd2);                

    for (int i = 0; i < count; ++i) {       
        fgets(line, 0x100, rd2);
        char* p = strstr(line, "=");
        *p = 0;
        events[i].start = static_cast<float>(atoi(line));
    }
    for (int i = 0; i < count; ++i) {       
        sprintf_s(idbuf, 0x100, "[ID#%04d]", i + 1);                
        if (strstr(line, idbuf) == nullptr) {
            do {
                fgets(line, 0x100, rd2);
            } while (strstr(line, idbuf) == nullptr);
        }
        if (strstr(line, "Length=") == nullptr) {                   
            do {
                fgets(line, 0x100, rd2);
            } while (strstr(line, "Length=") == nullptr);
        }
        char* p = strstr(line, "=");
        events[i].end = static_cast<float>(atoi(p + 1));
        if (strstr(line, "LyricHandle=") == nullptr) {              
            do {
                fgets(line, 0x100, rd2);
            } while (strstr(line, "LyricHandle=") == nullptr);
        }
        p = strstr(line, "=");
        sprintf_s(events[i].handle, 10, p + 1);
        events[i].handle[6] = 0;
    }

    for (int j = 0; j < count; ++j) {
        VsqEvent& ev = events[j];
        if (strstr(line, ev.handle) == nullptr) {
            do {
                fgets(line, 0x100, rd2);
            } while (strstr(line, ev.handle) == nullptr);
        }
        if (j == 0xae) {
        }
        fgets(line, 0x100, rd2);            
        char* m = strstr(line, "\",\"");                             
        *m = 0;                    
        char* firstQuote = strstr(line, "\"");                      
        sprintf_s(reinterpret_cast<char*>(&ev) + 7, 8, firstQuote + 1);

        const char* m1 = m + 1;              
        if (m1[3] == '"') {                  
            ev.consonant[0] = 0;
            ev.vowel = m[3];
        } else if (m1[3] == '\\') {          
            if (m[5] == ' ') {
                m[5] = 0;
                sprintf_s(ev.consonant, 6, m + 3);
                ev.vowel = m[6];
            } else {
                ev.consonant[0] = 0;        
                ev.vowel = 'N';
            }
        } else {
            char* space = strstr(const_cast<char*>(m1), " ");        
            if (space == nullptr) {
                ev.consonant[0] = 0;
                ev.vowel = 'N';
            } else {
                *space = 0;
                sprintf_s(ev.consonant, 6, m + 3);
                ev.vowel = space[1];
            }
        }
        if (ev.vowel == 'M')                                         
            ev.vowel = 'u';                                          
    }
    fclose(rd2);                                                    

    const float xdiv = static_cast<float>(division);                
    const double kMicro = 9.999999974752427e-07;   
    for (int i = 0; i < count; ++i) {
        VsqEvent& ev = events[i];
        int tick = static_cast<int>(ev.start);   
        float sec = 0.0f;
        int ci = 0;
        auto segAdd = [&](int ticks, int tempo) {
            sec = static_cast<float>(
                sec + static_cast<double>(ticks) / xdiv *
                          static_cast<double>(tempo) * kMicro);
        };
        if (tempoCount > 0) {
            for (;;) {
                if (ci + 1 == tempoCount) {                         
                    segAdd(tick, tempoTable[ci].tempo);
                    break;
                }
                if (tick <= tempoTable[ci + 1].time) {              
                    segAdd(tick, tempoTable[ci].tempo);
                    break;
                }
                const int t = tempoTable[ci + 1].time;
                segAdd(t, tempoTable[ci].tempo);
                tick -= t;
                ++ci;
                if (ci >= tempoCount)
                    break;
            }
        }
        ev.start = sec;                                             
        const float len = ev.end;
        ev.end = sec +
                 static_cast<float>(kMicro *
                     (static_cast<double>(tempoTable[ci].tempo) *
                      (static_cast<double>(len) / xdiv)));          

        char* token = ev.consonant;
        if (strncmp(token, "k'", 3) == 0) {                          
            if (ev.vowel == 'a')      sprintf_s(token, 6, "K");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "k");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "K");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "K");
        }
        if (strncmp(token, "S", 2) == 0) {                           
            if (ev.vowel == 'a')      sprintf_s(token, 6, "S");
            if (ev.vowel == 'i')      sprintf_s(token, 6, "s");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "S");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "S");
        }
        if (strncmp(token, "tS", 3) == 0) {                          
            if (ev.vowel == 'a')      sprintf_s(token, 6, "T");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "t");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "T");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "T");
        }
        if (strncmp(token, "J", 2) == 0) {                           
            if (ev.vowel == 'a')      sprintf_s(token, 6, "N");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "n");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "N");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "N");
        }
        if (strncmp(token, "C", 2) == 0) {                           
            if (ev.vowel == 'a')      sprintf_s(token, 6, "H");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "h");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "H");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "H");
        }
        if (strncmp(token, "m'", 3) == 0) {                          
            if (ev.vowel == 'a')      sprintf_s(token, 6, "M");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "m");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "M");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "M");
        }
        if (strncmp(token, "4'", 3) == 0) {                          
            if (ev.vowel == 'a')      sprintf_s(token, 6, "R");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "r");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "R");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "R");
        }
        if (strncmp(token, "g'", 3) == 0) {                          
            if (ev.vowel == 'a')      sprintf_s(token, 6, "G");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "g");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "G");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "G");
        }
        if (strncmp(token, "dZ", 3) == 0) {                          
            if (ev.vowel == 'a')      sprintf_s(token, 6, "Z");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "z");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "Z");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "Z");
        }
        if (strncmp(token, "b'", 3) == 0) {                          
            if (ev.vowel == 'a')      sprintf_s(token, 6, "B");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "b");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "B");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "B");
        }
        if (strncmp(token, "p'", 3) == 0) {                          
            if (ev.vowel == 'a')      sprintf_s(token, 6, "P");     
            if (ev.vowel == 'i')      sprintf_s(token, 6, "p");     
            if (ev.vowel == 'u')      sprintf_s(token, 6, "P");
            if (ev.vowel == 'o')      sprintf_s(token, 6, "P");
        }
        if (strncmp(token, "4", 2) == 0)                             
            sprintf_s(token, 6, "r");                                
        if (strncmp(token, "j", 2) == 0)                             
            sprintf_s(token, 6, "y");                                
        if (strncmp(token, "p\\", 3) == 0)                           
            sprintf_s(token, 6, "h");                                
        if (strncmp(token, "dz", 3) == 0)                            
            sprintf_s(token, 6, "z");                                
        if (strncmp(token, "ts", 3) == 0)                            
            sprintf_s(token, 6, "t");                                
        if (ev.vowel == 'm') ev.vowel = 'n';                         
        if (ev.vowel == 'N') ev.vowel = 'n';                         
    }

    FILE* out = nullptr;
    _wfopen_s(&out, path, L"wt");                                   
    for (int i = 0; i < count; ++i) {
        VsqEvent& ev = events[i];
        char* token = reinterpret_cast<char*>(&ev) + 7;
        sprintf_s(buf, 1000,
                  "%7.3f -%7.3f   %-4s %1s %c",                     
                  static_cast<double>(ev.start),
                  static_cast<double>(ev.end),
                  token, ev.consonant,
                  static_cast<char>(ev.vowel));
        fprintf(out, "%s\n", buf);                                  
    }
    fclose(out);                                                    

    unsigned char* model = SlotModel(app);
    mdl::ModelRecord& modelRecord = *mdl::Mdl(model);
    const int faceCount = modelRecord.morphCount;
    int faceA = 0, faceI = 0, faceU = 0, faceO = 0;
    if (faceCount <= 0) {
        VsqBox(app,
               "Cannot Lipsync because this model has not facial "
               "data of 'a'!!",                                    
               kJpNoA, "lipsync", kJpVsqCaption);                  
        return;
    }
    mdl::MorphRecord* const morphs = mdl::Morphs(model);
    {
        int i = 0;
        for (; i < faceCount; ++i) {
            if (memcmp(morphs[i].name, kFaceA, 3) == 0) break;
        }
        if (i >= faceCount) {
            VsqBox(app,
                   "Cannot Lipsync because this model has not facial "
                   "data of 'a'!!",
                   kJpNoA, "lipsync", kJpVsqCaption);
            return;
        }
        faceA = i;
    }
    {
        int i = 0;
        for (; i < faceCount; ++i) {
            if (memcmp(morphs[i].name, kFaceI, 3) == 0) break;
        }
        if (i >= faceCount) {
            VsqBox(app,
                   "Cannot Lipsync because this model has not facial "
                   "data of 'i'!!",                                
                   kJpNoI, "lipsync", kJpVsqCaption);
            return;
        }
        faceI = i;
    }
    {
        int i = 0;
        for (; i < faceCount; ++i) {
            if (memcmp(morphs[i].name, kFaceU, 3) == 0) break;
        }
        if (i >= faceCount) {
            VsqBox(app,
                   "Cannot Lipsync because this model has not facial "
                   "data of 'u'!!",                                
                   kJpNoU, "lipsync", kJpVsqCaption);
            return;
        }
        faceU = i;
    }
    {
        int i = 0;
        for (; i < faceCount; ++i) {
            if (memcmp(morphs[i].name, kFaceO, 3) == 0) break;
        }
        if (i >= faceCount) {
            VsqBox(app,
                   "Cannot Lipsync because this model has not facial "
                   "data of 'o'!!",                                
                   kJpNoO, "lipsync", kJpVsqCaption);
            return;
        }
        faceO = i;
    }

    mdl::BoneKey* const boneKeys = mdl::BoneKeys(model);
    for (int i = 0; i < static_cast<int>(mdl::kBoneKeyCapacity); ++i)
        boneKeys[i].allocated = 0;
    mdl::MorphKey* const morphKeys = mdl::MorphKeys(model);
    for (int i = 0; i < 20000; ++i)
        morphKeys[i].allocated = 0;
    mdl::DisplayKey* const displayKeys = mdl::DisplayKeys(model);
    for (int i = 0; i < 1000; ++i)
        displayKeys[i].allocated = 0;

    for (int i = 0; i < count; ++i) {
        VsqEvent& ev = events[i];
        const double start = ev.start;
        const double end = ev.end;
        if (ev.vowel == 'h')
            ev.vowel = ev.consonant[0];
        const int f1 = -2 - static_cast<int>(start * -30.0);        
        const int f2 = static_cast<int>(start * 30.0);              
        const int f3 = -2 - static_cast<int>(end * -30.0);
        const int f4 = static_cast<int>(end * 30.0);
        if (ev.vowel == 'a') {                                      
            VsqMorphKey(model, morphs, faceA, f1, 0.0f);         
            VsqMorphKey(model, morphs, faceA, f2, 0.7f);
            VsqMorphKey(model, morphs, faceA, f3, 0.7f);
            VsqMorphKey(model, morphs, faceA, f4, 0.0f);
        } else if (ev.vowel == 'i') {                               
            VsqMorphKey(model, morphs, faceI, f1, 0.0f);         
            VsqMorphKey(model, morphs, faceI, f2, 0.6f);
            VsqMorphKey(model, morphs, faceI, f3, 0.6f);
            VsqMorphKey(model, morphs, faceI, f4, 0.0f);
        } else if (ev.vowel == 'u') {                               
            VsqMorphKey(model, morphs, faceU, f1, 0.0f);
            VsqMorphKey(model, morphs, faceU, f2, 1.0f);
            VsqMorphKey(model, morphs, faceU, f3, 1.0f);
            VsqMorphKey(model, morphs, faceU, f4, 0.0f);
        } else if (ev.vowel == 'e') {                               
            VsqMorphKey(model, morphs, faceA, f1, 0.0f);
            VsqMorphKey(model, morphs, faceI, f1, 0.0f);
            VsqMorphKey(model, morphs, faceA, f2, 0.5f);         
            VsqMorphKey(model, morphs, faceI, f2, 0.3f);         
            VsqMorphKey(model, morphs, faceA, f3, 0.5f);
            VsqMorphKey(model, morphs, faceI, f3, 0.3f);
            VsqMorphKey(model, morphs, faceA, f4, 0.0f);
            VsqMorphKey(model, morphs, faceI, f4, 0.0f);
        } else if (ev.vowel == 'o') {                               
            VsqMorphKey(model, morphs, faceO, f1, 0.0f);
            VsqMorphKey(model, morphs, faceO, f2, 1.0f);
            VsqMorphKey(model, morphs, faceO, f3, 1.0f);
            VsqMorphKey(model, morphs, faceO, f4, 0.0f);
        }
    }

    if (count > 0) {
        const float lastEnd =
            events[count - 1].end;
        s.state.currentFrame =
            static_cast<int>(lastEnd * 30.0);
    }
    const int frameNow = s.state.currentFrame;
    const std::uint32_t modelMax = modelRecord.maxFrame;
    if (s.LastRegisteredFrame() < modelMax)
        s.LastRegisteredFrame() = modelMax;

    HWND frameEdit = GetDlgItem(hwnd, panel::kCurrentFrameEdit);                       
    SendMessageA(frameEdit, EM_SETSEL, 0,
                 GetWindowTextLengthA(frameEdit));                  
    sprintf_s(buf, 1000, "%d", frameNow);                           
    frameEdit = GetDlgItem(hwnd, panel::kCurrentFrameEdit);
    SendMessageA(frameEdit, EM_REPLACESEL, 0,
                 reinterpret_cast<LPARAM>(buf));                    

    SeekModelFrame(model, frameNow, s.PlaybackPhysicsMode());             

    s.state.timelineStartFrame =
        frameNow > 6 ? frameNow - 6 : 0;
    PanelPaint(app);                                                

    if (s.state.waveEnabled != 0) {
        TimelineDrawTicks(s.state.timelineStartFrame,
                          s.state.sidebarWidth);
        RECT rc;
        rc.left = 6;
        rc.top = 0x5F;
        rc.right = static_cast<LONG>(
            s.state.sidebarWidth) - 3;
        rc.bottom = 0x92;
        InvalidateRect(hwnd, &rc, 0);                               
    }
    if (s.state.aviBackgroundEnabled == 1)
        AviBgOverlayRefresh(app);                                   
}

}  

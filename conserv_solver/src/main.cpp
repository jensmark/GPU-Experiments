//
//  main.cpp
//  GLAppNative
//
//  Created by Jens Kristoffer Reitan Markussen on 28.12.13.
//  Copyright (c) 2013 Jens Kristoffer Reitan Markussen. All rights reserved.
//

#include "AppManager.h"

#include <stdlib.h>
#include "optionparser.h"

enum  optionIndex {UNKNOWN, HELP, TIME,
                X_SIZE, Y_SIZE, N_SIZE,
                SOLVER, DEVICE};

const option::Descriptor usage[] =
{
    {HELP,      0,"", "help",   option::Arg::None,        "  --help  \tPrint usage and exit.\n" },
    {TIME,      0,"", "time",   option::Arg::Optional,    "  --time  \tSet the total simulation time."},
    {X_SIZE,    0,"", "xn",     option::Arg::Optional,    "  --xn  \tSet the grid size in X-direction."},
    {Y_SIZE,    0,"", "yn",     option::Arg::Optional,    "  --yn  \tSet the grid size in Y-direction."},
    {N_SIZE,    0,"", "nt",     option::Arg::Optional,    "  --nt  \tSet the max simulation steps."},
    {SOLVER,    0,"", "type",   option::Arg::Optional,    "  --type  \tSet Solver type [CLSW,GLEULER,CLEULER]. REQUIRED."},
    {DEVICE,    0,"", "device", option::Arg::Optional,    "  --device  \tSet the perferred device [CPU,GPU], ignored if OpenGL solver"},
    
    {UNKNOWN, 0,"" ,  ""   ,option::Arg::None, "" },
    {0,0,0,0,0,0}
};

template<typename T>
T setValue(option::Option* options, optionIndex index, T def)
{
    if (options[index] == NULL || options[index].arg == NULL) {
        return def;
    }else{
        return std::atof(options[index].arg);
    }
}

Solver stringToEnum(const char* str){
    std::string txt(str);
    if (txt.compare("CLSW") == 0) {
        return CL_SW;
    } else if (txt.compare("GLEULER") == 0) {
        return GL_EULER;
    } else if (txt.compare("CLEULER") == 0) {
        return CL_EULER;
    } else {
        return UNKNOWN_SOLVER;
    }
}

int main(int argc, const char * argv[])
{
    argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
    option::Stats  stats(usage, argc, argv);
    option::Option* options = new option::Option[stats.options_max];
    option::Option* buffer = new option::Option[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);
    
    if (parse.error())
        return 1;
    
    if (options[HELP]) {
        option::printUsage(std::cout, usage);
        return 0;
    }
    
    if (options[SOLVER] == NULL || options[SOLVER].arg == NULL) {
        std::cout << std::endl << "\tSolver type is required!" << std::endl << std::endl;
        option::printUsage(std::cout, usage);
        return 0;
    }
    
    for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next()){
        std::cout << "Unknown option: " << opt->name << "\n";
    }
    
    float time;
    size_t Nx, Ny, N;
    
    time    = setValue<float>(options,TIME,0.2f);
    Nx      = setValue<size_t>(options,X_SIZE,128);
    Ny      = setValue<size_t>(options,Y_SIZE,128);
    N       = setValue<size_t>(options,N_SIZE,150);
    
    
    AppManager* manager = NULL;
    try {
        manager = new AppManager();
        manager->init(Nx,Ny,stringToEnum(options[SOLVER].arg),options[DEVICE].arg);
        manager->begin(N,time);
        
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    delete manager;
    
    delete [] options;
    delete [] buffer;
    
    return 0;
}


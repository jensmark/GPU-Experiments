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

enum  optionIndex {UNKNOWN, HELP, MODEL};

const option::Descriptor usage[] =
{
    {HELP,      0,"", "help",   option::Arg::None,        "\t--help  \tPrint usage and exit.\n\tRequired:" },
    {MODEL,      0,"", "model",   option::Arg::Optional,      "\t--model=bunny  \tSet the model to load and render." },
    
    {UNKNOWN, 0,"" ,  ""   ,option::Arg::None, "" },
    {0,0,0,0,0,0}
};

int main(int argc, const char * argv[])
{
    argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
    option::Stats  stats(usage, argc, argv);
    option::Option* options = new option::Option[stats.options_max];
    option::Option* buffer = new option::Option[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);
    
    if (parse.error())
        return 1;
    
    if (options[HELP] || argc == 0) {
        option::printUsage(std::cout, usage);
        return 0;
    }
    
    for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next()){
        std::cout << "Unknown option: " << opt->name << "\n";
    }
    
    AppManager* manager = NULL;
    try {
        manager = new AppManager();
        manager->init(options[MODEL].arg);
        manager->begin();
        
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    delete manager;
    
    delete [] options;
    delete [] buffer;
    
    return 0;
}


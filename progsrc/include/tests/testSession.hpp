/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

#include <core/smileCommon.hpp>
#include <core/dataMemory.hpp>
#include <core/configManager.hpp>
#include <core/componentManager.hpp>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>

// Encapsulates cConfigManager and cComponentManager instances with utility methods for usage in unit tests.
class TestSession {
  private:
    cConfigManager confman;
    cComponentManager cMan;
    cDataMemory *dm;

    // creates and registers a cDataMemory instance with name "dataMemory"
    cDataMemory *addDataMemory();

  public:
    TestSession();

    cConfigManager &getConfigManager() { return confman; }
    cComponentManager &getComponentManager() { return cMan; }
    cDataMemory &getDataMemory() { return *dm; }

    // Add or update an existing ConfigInstance with the specified instance name and component type.
    // In the callback, you may set values of the ConfigInstance by using the ConfigInstance::setInt/setDouble/etc. methods.
    //
    // E.g. the config file section:
    //
    // [energy:cEnergy]
    // reader.dmLevel = input
    // writer.dmLevel = output
    //
    // is equivalent to calling:
    //
    // addConfigInstance(confman, "energy", "cEnergy", [](ConfigInstance &ci) {
    //   ci.setStr("reader.dmLevel[0]", "input");
    //   ci.setStr("writer.dmLevel", "output");
    // });
    void addConfigInstance(const char *name, const char *type, std::function<void(ConfigInstance&)> callback);

    // Creates a data memory level using the specified level config.
    // In the callback, you need to set up the level by calling cDataMemory::addField.
    //
    // Use this method when you want to push test input data into the openSMILE
    // pipeline from your test case. (Alternatively, you may use cExternal(Audio)Source.)
    //
    // Sample usage:
    //
    // sDmLevelConfig cfg("input", 1.0 / 16000, 1.0 / 16000, 100L);
    // addLevel(*dm, cfg, [dm](int level) {
    //   dm->addField(level, "pcm", 4);
    // });
    int addLevel(sDmLevelConfig &lcfg, std::function<void(cDataMemory&, int)> callback);

    // Creates all specified components and configures and finalises the component graph.
    // Typically, you call this with all instances that you have created ConfigInstances for.
    void addComponents(const std::vector<const char *> &instnames);

    // Sets the content of the specified data memory level from a matrix. 
    // Use this function for levels that you have created with addLevel.
    // Currently, this function may only be called once for each level, thus the full
    // input needs to be present in the matrix.
    void setInput(const char *levelname, const cMatrix &mat);

    // Gets the full content of the specified data memory level as a matrix.
    // Use this function to retrieve all data written to a level by a component.
    std::unique_ptr<cMatrix> getOutput(const char *levelname);

    // Sets the content of the specified input level, runs the tick loop
    // and after completion, returns the content of the specified output level.
    //
    // Use this function to quickly obtain the output of a component based on a certain input.
    std::unique_ptr<cMatrix> process(const char *inputlevel, const cMatrix &input, const char *outputlevel);

    // Sets the content of the specified input levels, runs the tick loop
    // and after completion, returns the content of the specified output levels.
    std::unordered_map<const char *, std::unique_ptr<cMatrix>> process(
        const std::unordered_map<const char *, const cMatrix &> &inputs, 
        const std::vector<const char *> outputlevels);
};
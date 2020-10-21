//
//  OpenSmile.m
//  usingSmileLib
//
//  Created by Taewoo Kim on 17.06.19.
//  Copyright © 2019 audEERING. All rights reserved.
//

#import "OpenSmile.h"
#import <Foundation/Foundation.h>
#import <smileapi/SMILEapi.h>

#import <core/componentManager.hpp>
#import <core/configManager.hpp>
#import <core/commandlineParser.hpp>
#import <iocore/externalSource.hpp>
#import <iocore/externalAudioSource.hpp>
#import <iocore/externalSink.hpp>
#import <ios/coreAudioSource.hpp>
#import <other/externalMessageInterface.hpp>

@class OpenSmile;

/**
 Callback function for receiving message from openSMILE
 
 @param msg Json format message from openSMILE.
 @param param Pointer to the dictionary which includes instance of OpenSmile and callback function.
 @return result
 */
BOOL externalMessageInterfaceJsonCallback (const char *msg, void *param)
{
    if(NULL == msg || NULL == param) {
        return NO;
    }
    
    NSDictionary* dic = (__bridge NSDictionary*)param;
    void (^callback)(NSString* msg) = [dic objectForKey: @"callback"];
    if(nil != callback) {
        callback([NSString stringWithCString: msg encoding: NSUTF8StringEncoding]);
    }
    return YES;
}

/**
 Callback function for receiving data from openSMILE
 
 @param data 32bit float data from openSMILE.
 @param vectorSize Size of the data.
 @param param Pointer to the dictionary which includes instance of OpenSmile and callback function.
 */
BOOL externalSinkCallback(const FLOAT_DMEM *data, long vectorSize, void *param)
{
    if(NULL == data || NULL == param) {
        return NO;
    }
    
    NSDictionary* dic = (__bridge NSDictionary*)param;
    void (^callback)(Float32* data, unsigned int length) = [dic objectForKey: @"callback"];
    if(nil != callback) {
        callback((Float32*)data, (unsigned int)vectorSize);
    }
    return YES;
}

@interface OpenSmile()
@property (nonatomic) smileobj_t *smileobj;
@property (strong, nonatomic) NSMutableArray *callbackInfoAry;
@end

@implementation OpenSmile


- (id)init {
    self = [super init];
    if (self) {
        [self class];
    }
    
    self.callbackInfoAry = [[NSMutableArray alloc] init];
    return self;
}

- (void) dealloc {
    [self releaseObjects];
}

/**
 Prepare OpenSmile to run
 
 @param configFile Config file name which is included in the bundle.
 @param args Arguments to the OpenSmile. ex: {"-l", "2", "-O", outwav}
 */
- (BOOL) initialize: (NSString*)configFile argument: (NSArray*)args error: (NSError **)error
{
    @try {
        unsigned long optionCount = args.count / 2;
        if (0 < optionCount) {
            smileopt_t smileopts[optionCount];
            int index = 0;
            for (NSString* arg in args) {
                if (0 == index % 2) {
                    smileopts[index / 2].name = (char*)arg.UTF8String;
                }
                else {
                    smileopts[index / 2].value = (char*)arg.UTF8String;
                }
                
                index ++;
            }
            
            smileobj_t* obj = smile_new();
            smile_initialize(obj, configFile.UTF8String, (int)optionCount, smileopts, 5, 1, 1);
            if (NULL == obj) {
                @throw [NSException exceptionWithName: @"OpenSmile.initialize" reason: nil userInfo: nil];
            }
            self.smileobj = obj;
            
        }
        else {
            smileobj_t* obj = smile_new();
            smile_initialize(obj, configFile.UTF8String, 0, NULL, 5, 1, 1);
            if (NULL == obj) {
                @throw [NSException exceptionWithName: @"OpenSmile.initialize" reason: nil userInfo: nil];
            }
            self.smileobj = obj;
        }
        
        return YES;
    }
    @catch(NSException* nsexcep) {
        *error = [NSError errorWithDomain: nsexcep.name code: 0 userInfo: nsexcep.userInfo];
        return NO;
    }
}

/**
 Run OpenSmile
 */
- (BOOL) run: (NSError **)error
{
    @try {
        smile_run(self.smileobj);
        [self.callbackInfoAry removeAllObjects];
        return YES;
    } @catch (NSException* nsexcep) {
        *error = [NSError errorWithDomain: nsexcep.name code: 0 userInfo: nsexcep.userInfo];
        return NO;
    }
}

/**
 Reset OpenSmile
 */
- (void) reset
{
    smile_reset(self.smileobj);
}

/**
 Quit OpenSmile
 */
- (void) abort
{
    smile_abort(self.smileobj);
}

/**
 Release from memory
 */
- (void) releaseObjects
{
    if (NULL != self.smileobj) {
        smile_free(self.smileobj);
        self.smileobj = NULL;
    }
}

/**
 Send 32bit float array to openSMILE
 
 @param componentName Component name of the openSMILE.
 @param data Pointer to a 32bit float array.
 @param length Length of array.
 @return Result of sending data.
 */
- (bool) writeDataToExternalSource:(NSString*)componentName data: (Float32*)data length: (unsigned int) length
{
    int result = smile_extsource_write_data(self.smileobj, componentName.UTF8String, data, length);
    return (0 < result);
}

/**
 Stop sending data
 
 @param componentName Component name of the openSMILE.
 */
- (void) setEndOfInputForExternalSource:(NSString*)componentName
{
    smile_extsource_set_external_eoi(self.smileobj, componentName.UTF8String);
}

/**
 Send audio data to openSMILE
 
 @param componentName Component name of the openSMILE.
 @param data Pointer to a audio data.
 @param length Length of the data.
 @return Result of sending data.
 */
- (bool) writeDataToExternalAudioSource:(NSString*)componentName data: (void*)data length: (unsigned int) length
{
    int result = smile_extaudiosource_write_data(self.smileobj, componentName.UTF8String, data, length);
    return (0 < result);
}

/**
 Stop sending audio data
 
 @param componentName Component name of the openSMILE.
 */
- (void) setEndOfInputForExternalAudioSource:(NSString*)componentName
{
    smile_extaudiosource_set_external_eoi(self.smileobj, componentName.UTF8String);
}

/**
 Set external callback function to receive 32bit float data from openSMILE
 
 @param componentName Component name of the openSMILE.
 @param completion Callback function which will be called when receiving data from openSMILE
 */
- (void) setExternalSinkCallback: (NSString*)componentName callback: (void (^)(Float32* data, unsigned int length))completion
{
    ExternalSinkCallback callback = externalSinkCallback;
    NSDictionary* callbackInfo = @{@"callback" : completion};
    smile_extsink_set_data_callback(self.smileobj, componentName.UTF8String, callback, (__bridge void *)callbackInfo);
    [self.callbackInfoAry addObject: callbackInfo];
}

/**
 Set external callback function to receive json format data from openSMILE
 
 @param componentName Component name of the openSMILE.
 @param completion Callback function which will be called when receiving data from openSMILE.
 */
- (void) setExternalJsonMessageInterfaceCallback: (NSString*)componentName callback: (void (^)(NSString* msg))completion
{
    ExternalMessageInterfaceJsonCallback callback = externalMessageInterfaceJsonCallback;
    NSDictionary* callbackInfo = @{@"callback" : completion};
    smile_extmsginterface_set_json_msg_callback(self.smileobj, componentName.UTF8String, callback, (__bridge void *)callbackInfo);
    [self.callbackInfoAry addObject: callbackInfo];
}

@end

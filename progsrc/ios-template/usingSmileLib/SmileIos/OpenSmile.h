//
//  OpenSmile.h
//  usingSmileLib
//
//  Created by Taewoo Kim on 17.06.19.
//  Copyright © 2019 audEERING. All rights reserved.
//

#ifndef OpenSmile_h
#define OpenSmile_h

#import <Foundation/Foundation.h>

//
// Interface between iOS app and openSMILE library.
//
@interface OpenSmile: NSObject

- (BOOL) initialize: (NSString*)configFile argument: (NSArray*)args error: (NSError **)error;
- (void) setExternalSinkCallback: (NSString*)componentName callback: (void (^)(Float32* data, unsigned int length))completion;
- (void) setExternalJsonMessageInterfaceCallback: (NSString*)componentName callback: (void (^)(NSString* msg))completion;
- (BOOL) run:(NSError **)error;
- (void) reset;
- (void) abort;
- (bool) writeDataToExternalSource:(NSString*)componentName data: (Float32*)data length: (unsigned int) length;
- (bool) writeDataToExternalAudioSource:(NSString*)componentName data: (void*)data length: (unsigned int) length;
- (void) setEndOfInputForExternalSource:(NSString*)componentName;
- (void) setEndOfInputForExternalAudioSource:(NSString*)componentName;

@end

#endif /* OpenSmile_h */

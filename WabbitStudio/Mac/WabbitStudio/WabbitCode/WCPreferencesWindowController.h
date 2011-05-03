//
//  WCPreferencesWindowController.h
//  WabbitStudio
//
//  Created by William Towe on 3/24/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "DBPrefsWindowController.h"


extern NSString* const kWCPreferencesWindowControllerGeneralIdentifier;
extern NSString* const kWCPreferencesWindowControllerAppearanceIdentifier;
extern NSString* const kWCPreferencesWindowControllerBuildingIdentifier;
extern NSString* const kWCPreferencesWindowControllerDebuggingIdentifier;
extern NSString* const kWCPreferencesWindowControllerEditorIdentifier;
extern NSString* const kWCPreferencesWindowControllerUpdatesIdentifier;
extern NSString* const kWCPreferencesWindowControllerAdvancedIdentifier;

@class WCTextView,WCFile;

@interface WCPreferencesWindowController : DBPrefsWindowController {
@private
    IBOutlet NSView *_general;
	IBOutlet NSView *_appearance;
	IBOutlet NSView *_building;
	IBOutlet NSView *_debugging;
	IBOutlet NSView *_editor;
	IBOutlet NSView *_advanced;
	IBOutlet NSView *_files;
	IBOutlet NSView *_hardware;
	IBOutlet NSView *_screenshots;
	
	IBOutlet WCTextView *_textView;
	IBOutlet NSScrollView *_scrollView;
	
	IBOutlet NSTableView *_tableView;
	IBOutlet NSTabView *_tabView;
	
	WCFile *_file;
}
- (IBAction)chooseEditorFont:(id)sender;
- (IBAction)chooseCustomBuildLocation:(id)sender;
- (IBAction)selectTextEncoding:(NSPopUpButton *)sender;
@end

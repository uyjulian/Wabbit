//
//  WCProject.h
//  WabbitCode
//
//  Created by William Towe on 3/17/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import <Cocoa/Cocoa.h>

extern NSString* const kWCProjectFileKey;
extern NSString* const kWCProjectVersionKey;
extern NSString* const kWCProjectBuildTargetsKey;

extern const CGFloat kWCProjectVersion;

extern NSString* const kWCProjectDataFileName;
extern NSString* const kWCProjectWrapperExtension;

extern NSString* const kWCProjectUTI;

extern NSString* const kWCProjectActiveBuildTargetDidChangeNotification;
extern NSString *const kWCProjectNumberOfBuildTargetsDidChangeNotification;
extern NSString *const kWCProjectNumberOfFilesDidChangeNotification;

extern NSString *const kWCProjectSettingsProjectFilesOutlineViewExpandedItemUUIDsKey;
extern NSString *const kWCProjectSettingsProjectWindowFrameKey;
extern NSString *const kWCProjectSettingsLeftVerticalSplitViewDividerPositionKey;

@class WCProjectFile,PSMTabBarControl,WCFileViewController,WCFile,WCBuildTarget,BWAnchoredButtonBar,WCProjectFilesOutlineViewController,WCProjectNavigationViewController,WCBuildMessagesViewController,WCBuildMessage,WCSymbolsViewController,WCSymbol,WCFindInProjectViewController,WCFindInProjectResult,WCProjectNavView,CTBadge;

@interface WCProject : NSDocument <NSSplitViewDelegate,NSOutlineViewDelegate,NSUserInterfaceValidations,NSOpenSavePanelDelegate,NSToolbarDelegate,NSWindowDelegate> {
@private
	IBOutlet PSMTabBarControl *_tabBarControl;
	IBOutlet BWAnchoredButtonBar *_rightButtonBar;
	IBOutlet NSSplitView *_splitView;
	IBOutlet NSView *_swapView;
	IBOutlet WCProjectNavView *_navBarControl;
	
	WCProjectFile *_projectFile; // the root of our file outline view
	NSMapTable *_filesToFileViewControllers; // maps files to sets of file view controllers
	NSCountedSet *_openFiles; // open count for each file
	
	NSTask *_buildTask; // SPASM
	NSMutableArray *_buildTargets;
	BOOL _isBuilding;
	NSMutableArray *_buildMessages; // errors and warnings for the current build task
	CTBadge *_errorBadge; // white on red badge for number of errors for the current build task
	CTBadge *_warningBadge; // white on orange badge for the number of warnings for the current build task
	
	NSMutableDictionary *_projectSettings;
	
	__weak WCProjectNavigationViewController *_currentViewController;
	WCProjectFilesOutlineViewController *_projectFilesOutlineViewController;
	WCBuildMessagesViewController *_buildMessagesViewController;
	WCSymbolsViewController *_symbolsViewController;
	WCFindInProjectViewController *_findInProjectViewController;
	
	NSSet *_cachedAbsoluteFilePaths;
	
	BOOL _ignoreUnsavedChanges;
	BOOL _ignoreUnsavedChangesForProjectClose;
}

@property (readonly,retain,nonatomic) WCProjectFile *projectFile;
@property (readonly,nonatomic) NSArray *textFiles;
@property (readonly,nonatomic) NSMapTable *filesToFileViewControllers;
@property (readonly,nonatomic) NSArray *symbols;
@property (readonly,nonatomic) NSArray *equateSymbols;
@property (readonly,nonatomic) NSArray *defineSymbols;
@property (readonly,nonatomic) NSArray *macroSymbols;
@property (readonly,nonatomic) NSArray *labelSymbols;
@property (readonly,nonatomic) NSDictionary *labelNamesToSymbols;
@property (readonly,nonatomic) NSDictionary *equateNamesToSymbols;
@property (readonly,nonatomic) NSDictionary *defineNamesToSymbols;
@property (readonly,nonatomic) NSDictionary *macroNamesToSymbols;
@property (readonly,nonatomic) WCFileViewController *selectedFileViewController;
@property (readonly,nonatomic) NSArray *buildTargets;
@property (readonly,nonatomic) NSMutableArray *mutableBuildTargets;
@property (assign,nonatomic) WCBuildTarget *activeBuildTarget;
@property (readonly,nonatomic) NSCountedSet *openFiles;
@property (readonly,nonatomic) NSMutableDictionary *projectSettings;
@property (assign,nonatomic) BOOL isBuilding;
@property (assign,nonatomic) WCProjectNavigationViewController *currentViewController;
@property (readonly,nonatomic) WCProjectFilesOutlineViewController *projectFilesOutlineViewController;
@property (readonly,nonatomic) WCBuildMessagesViewController *buildMessagesViewController;
@property (readonly,nonatomic) WCSymbolsViewController *symbolsViewController;
@property (readonly,nonatomic) WCFindInProjectViewController *findInProjectViewController;
@property (readonly,nonatomic) WCFindInProjectViewController *findInProjectViewControllerDontCreate;
@property (readonly,nonatomic) NSArray *buildMessages;
@property (readonly,nonatomic) NSMutableArray *mutableBuildMessages;
@property (readonly,retain,nonatomic) NSSet *absoluteFilePaths;
@property (readonly,nonatomic) PSMTabBarControl *tabBarControl;
@property (readonly,nonatomic) NSArray *unsavedTextFiles;
@property (assign,nonatomic) BOOL ignoreUnsavedChanges;
@property (assign,nonatomic) BOOL ignoreUnsavedChangesForProjectClose;

- (IBAction)addFilesToProject:(id)sender;
- (IBAction)newFile:(id)sender;

- (IBAction)build:(id)sender;

- (IBAction)editBuildTargets:(id)sender;

- (IBAction)newGroup:(id)sender;
- (IBAction)group:(id)sender;
- (IBAction)ungroup:(id)sender;

- (IBAction)viewProject:(id)sender;
- (IBAction)viewBuildMessages:(id)sender;
- (IBAction)viewSymbols:(id)sender;
- (IBAction)viewSearch:(id)sender;
- (IBAction)changeProjectView:(id)sender;

- (WCFileViewController *)addFileViewControllerForFile:(WCFile *)file;
- (WCFileViewController *)selectFileViewControllerForFile:(WCFile *)file;
- (void)removeFileViewControllerForFile:(WCFile *)file;

- (NSArray *)symbolsForSymbolName:(NSString *)name;

- (void)removeAllBuildMessages;
- (void)jumpToBuildMessage:(WCBuildMessage *)message;
- (void)jumpToSymbol:(WCSymbol *)symbol;
- (void)jumpToFindInProjectResult:(WCFindInProjectResult *)findResult;
@end
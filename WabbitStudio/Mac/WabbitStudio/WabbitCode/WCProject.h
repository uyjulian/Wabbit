//
//  WCProject.h
//  WabbitCode
//
//  Created by William Towe on 3/17/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import <AppKit/NSDocument.h>
#import "WCTabViewContextProtocol.h"
#import "WCJumpToObjectProtocol.h"

extern NSString *const kWCProjectFileKey;
extern NSString *const kWCProjectVersionKey;
extern NSString *const kWCProjectBuildTargetsKey;

extern const CGFloat kWCProjectVersion;

extern NSString *const kWCProjectDataFileName;
extern NSString *const kWCProjectWrapperExtension;

extern NSString *const kWCProjectUTI;

extern NSString *const kWCProjectActiveBuildTargetDidChangeNotification;
extern NSString *const kWCProjectNumberOfBuildTargetsDidChangeNotification;
extern NSString *const kWCProjectNumberOfFilesDidChangeNotification;
extern NSString *const kWCProjectWillCloseNotification;

extern NSString *const kWCProjectSettingsProjectFilesOutlineViewExpandedItemUUIDsKey;
extern NSString *const kWCProjectSettingsProjectWindowFrameKey;
extern NSString *const kWCProjectSettingsLeftVerticalSplitViewDividerPositionKey;
extern NSString *const kWCProjectSettingsOpenFileUUIDsKey;
extern NSString *const kWCProjectSettingsSelectedFileUUIDKey;
extern NSString *const kWCProjectSettingsFileSettingsDictionaryKey;
extern NSString *const kWCProjectSettingsFileSettingsFileSeparateEditorWindowFrameKey;

@class WCProjectFile,PSMTabBarControl,WCFileViewController,WCFile,WCBuildTarget,BWAnchoredButtonBar,WCProjectFilesOutlineViewController,WCProjectNavigationViewController,WCBuildMessagesViewController,WCBuildMessage,WCSymbolsViewController,WCSymbol,WCFindInProjectViewController,WCFindInProjectResult,WCProjectNavView,CTBadge,WCBreakpointsViewController,WCBreakpoint,WCAddFilesToProjectViewController;

@interface WCProject : NSDocument <NSSplitViewDelegate,NSOutlineViewDelegate,NSUserInterfaceValidations,NSOpenSavePanelDelegate,NSToolbarDelegate,NSWindowDelegate,NSTabViewDelegate,WCTabViewContext> {
@private
	IBOutlet PSMTabBarControl *_tabBarControl;
	IBOutlet BWAnchoredButtonBar *_rightButtonBar;
	IBOutlet NSSplitView *_splitView;
	IBOutlet NSView *_swapView;
	IBOutlet WCProjectNavView *_navBarControl;
	
	WCProjectFile *_projectFile; // the root of our file outline view
	NSMapTable *_filesToFileViewControllers; // maps files to sets of file view controllers
	NSCountedSet *_openFiles; // open count for each file so we know when to prompt to save if the project is being closed
	
	NSTask *_buildTask; // SPASM
	NSMutableArray *_buildTargets;
	BOOL _isBuilding;
	NSMutableArray *_buildMessages; // errors and warnings for the current build task
	CTBadge *_errorBadge; // white on red badge for number of errors for the current build task
	CTBadge *_warningBadge; // white on orange badge for the number of warnings for the current build task
	NSString *_codeListing; // lst file that SPASM spit out for the last successful build
	
	WCBreakpoint *_projectBreakpoint; // root breakpoint that coordinates displaying all the breakpoints in the
									  // breakpoints view on the left
	
	NSMutableDictionary *_projectSettings; // we keep this updated when things change in the project and write it out
										   // with each save as <username>.wcodesettings
	
	__weak WCProjectNavigationViewController *_currentViewController; // current view displayed on the left
	WCProjectFilesOutlineViewController *_projectFilesOutlineViewController; // main files view
	WCBuildMessagesViewController *_buildMessagesViewController; // errors warnings from the current build
	WCSymbolsViewController *_symbolsViewController; // all symbols in the project by file
	WCFindInProjectViewController *_findInProjectViewController; // search and replace in the project
	WCBreakpointsViewController *_breakpointsViewController; // all breakpoints in the project
	
	NSSet *_cachedAbsoluteFilePaths; // to validate files being added from the addFilesToProject: method
	WCAddFilesToProjectViewController *_currentAddFilesToProjectViewController;
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
@property (readonly,nonatomic) WCBreakpointsViewController *breakpointsViewController;
@property (readonly,nonatomic) WCBreakpointsViewController *breakpointsViewControllerDontCreate;
@property (readonly,nonatomic) NSArray *buildMessages;
@property (readonly,nonatomic) NSMutableArray *mutableBuildMessages;
@property (readonly,retain,nonatomic) NSSet *absoluteFilePaths;
@property (readonly,nonatomic) NSArray *unsavedTextFiles;
@property (readonly,nonatomic) id <WCTabViewContext> currentTabViewContext;
@property (readonly,nonatomic) NSArray *fileWindowControllers;
@property (readonly,retain,nonatomic) NSString *codeListing;
@property (readonly,nonatomic) WCBreakpoint *projectBreakpoint;

- (IBAction)addFilesToProject:(id)sender;
- (IBAction)newFile:(id)sender;

- (IBAction)build:(id)sender;

- (IBAction)editBuildTargets:(id)sender;

- (IBAction)newGroup:(id)sender;
- (IBAction)group:(id)sender;
- (IBAction)ungroup:(id)sender;

- (IBAction)rename:(id)sender;

- (IBAction)showInFinder:(id)sender;
- (IBAction)openWithFinder:(id)sender;

- (IBAction)viewProject:(id)sender;
- (IBAction)viewBuildMessages:(id)sender;
- (IBAction)viewSymbols:(id)sender;
- (IBAction)viewSearch:(id)sender;
- (IBAction)viewBreakpoints:(id)sender;
- (IBAction)changeProjectView:(id)sender;

- (IBAction)nextTab:(id)sender;
- (IBAction)previousTab:(id)sender;

- (IBAction)openInSeparateEditor:(id)sender;

- (IBAction)projectWindow:(id)sender;

- (WCFileViewController *)addFileViewControllerForFile:(WCFile *)file inTabViewContext:(id <WCTabViewContext>)tabViewContext;
- (WCFileViewController *)fileViewControllerForFile:(WCFile *)file inTabViewContext:(id <WCTabViewContext>)tabViewContext selectTab:(BOOL)selectTab;
- (void)removeFileViewControllerForFile:(WCFile *)file inTabViewContext:(id <WCTabViewContext>)tabViewContext;

- (NSArray *)symbolsForSymbolName:(NSString *)name;

- (void)removeAllBuildMessages;
- (void)jumpToObject:(id <WCJumpToObject>)object;
- (void)jumpToObjects:(NSArray *)objects;

- (void)saveProjectFile;

- (void)noteNumberOfFilesChanged;
@end

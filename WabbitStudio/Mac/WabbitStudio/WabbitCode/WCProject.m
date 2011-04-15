//
//  WCProject.m
//  WabbitCode
//
//  Created by William Towe on 3/17/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCProject.h"
#import "NSArray+WCExtensions.h"
#import "WCProjectFile.h"
#import "WCIconTextFieldCell.h"
#import "NSFileManager+WCExtensions.h"
#import "WCGeneralPerformer.h"
#import "NSOutlineView+WCExtensions.h"
#import "WCAddFilesToProjectViewController.h"
#import "WCFileViewController.h"
#import "NSTreeController+WCExtensions.h"
#import "WCSymbolScanner.h"
#import "WCTextView.h"
#import "WCSymbol.h"
#import "WCTextStorage.h"
#import "WCBuildMessage.h"
#import "WCBuildTarget.h"
#import "WCBuildTargetInfoSheetController.h"
#import "WCPreferencesController.h"
#import "NSUserDefaults+WCExtensions.h"
#import "WCDefines.h"
#import "WCAlias.h"
#import "WCBuildDefine.h"
#import "WCIncludeDirectory.h"
#import "NSIndexPath+WCExtensions.h"
#import "WCProjectFilesOutlineViewController.h"
#import "WCBuildMessagesViewController.h"
#import "WCSymbolsViewController.h"
#import "WCSymbol.h"
#import "WCNewFileWindowController.h"
#import "WCFindInProjectViewController.h"
#import "WCProjectBuildTargetPopUpButton.h"
#import "WCFindInProjectResult.h"
#import "WCProjectNavView.h"
#import "NSAlert-OAExtensions.h"
#import "WCUnsavedFilesWindowController.h"
#import "CTBadge.h"
#import "NSTextView+WCExtensions.h"
#import "WCSyntaxHighlighter.h"

#import <PSMTabBarControl/PSMTabBarControl.h>
#import <BWToolkitFramework/BWToolkitFramework.h>

NSString *const kWCProjectFileKey = @"file";
NSString *const kWCProjectVersionKey = @"version";
NSString *const kWCProjectBuildTargetsKey = @"buildTargets";

const CGFloat kWCProjectVersion = 1.5;

NSString *const kWCProjectDataFileName = @"project.wcodedata";
NSString *const kWCProjectWrapperExtension = @"wcodeproj";

NSString *const kWCProjectUTI = @"org.revsoft.wabbitcode.project";

NSString *const kWCProjectActiveBuildTargetDidChangeNotification = @"kWCProjectActiveBuildTargetDidChangeNotification";
NSString *const kWCProjectNumberOfBuildTargetsDidChangeNotification = @"kWCProjectNumberOfBuildTargetsDidChangeNotification";
NSString *const kWCProjectNumberOfFilesDidChangeNotification = @"kWCProjectNumberOfFilesDidChangeNotification";

NSString *const kWCProjectSettingsProjectFilesOutlineViewExpandedItemUUIDsKey = @"projectFilesOutlineViewExpandedItemUUIDs";
NSString *const kWCProjectSettingsProjectWindowFrameKey = @"projectWindowFrame";
NSString *const kWCProjectSettingsLeftVerticalSplitViewDividerPositionKey = @"leftVerticalSplitViewDividerPosition";
NSString *const kWCProjectSettingsOpenFileUUIDsKey = @"projectOpenFileUUIDs";
NSString *const kWCProjectSettingsSelectedFileUUIDKey = @"projectSelectedFileUUID";

static NSString *const kWCProjectErrorDomain = @"org.revsoft.wabbitcode.project.error";
static const NSInteger kWCProjectDataFileMovedErrorCode = 1001;
static const NSInteger kWCProjectDataFileIncorrectFormatErrorCode = 1002;
static const NSInteger kWCProjectDataFileOldVersionErrorCode = 1003;

static NSImage *_appIcon = nil;

@interface WCProject ()
@property (readwrite,retain,nonatomic) WCProjectFile *projectFile;
@property (readwrite,retain,nonatomic) NSSet *absoluteFilePaths;

- (void)_addBuildMessageForString:(NSString *)string;

- (void)_updateProjectSettings;
- (void)_applyProjectSettings;

- (void)_deleteObjects:(NSArray *)objects deleteFiles:(BOOL)deleteFiles;
@end

@implementation WCProject
#pragma mark -
#pragma mark *** Subclass Overrides ***
+ (void)initialize {
	if ([WCProject class] != self)
		return;
	
	_appIcon = [[NSImage imageNamed:@"NSApplicationIcon"] copy];
}

- (void)dealloc {
#ifdef DEBUG
	NSLog(@"%@ called in %@",NSStringFromSelector(_cmd),[self className]);
#endif
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_errorBadge release];
	[_warningBadge release];
	_currentViewController = nil;
	[_cachedAbsoluteFilePaths release];
	[_buildMessages release];
	[_projectFilesOutlineViewController release];
	[_buildMessagesViewController release];
	[_symbolsViewController release];
	[_findInProjectViewController release];
	[_buildTask release];
	[_projectSettings release];
	[_openFiles release];
	[_filesToFileViewControllers release];
	[_buildTargets release];
	[_projectFile release];
	[super dealloc];
}

- (NSString *)windowNibName {
	return @"WCProject";
}

- (void)windowControllerDidLoadNib:(NSWindowController *)controller {
	[super windowControllerDidLoadNib:controller];
	// Add any code here that needs to be executed once the windowController has loaded the document's window.
	
	[_tabBarControl setCanCloseOnlyTab:YES];
	[_tabBarControl setStyleNamed:@"Unified"];
	[_tabBarControl setHideForSingleTab:NO];
	[_tabBarControl setUseOverflowMenu:YES];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_fileHasUnsavedChanges:) name:kWCFileHasUnsavedChangesNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_fileNameDidChange:) name:kWCFileNameDidChangeNotification object:nil];

	[_rightButtonBar setIsAtBottom:YES];
	[_rightButtonBar setIsResizable:NO];
	
	[self setCurrentViewController:[self projectFilesOutlineViewController]];
	
	[self _applyProjectSettings];
	
	NSString *identifier = [NSString stringWithFormat:@"%@ToolbarIdentifier",[self className]];
	NSToolbar *toolbar = [[[NSToolbar alloc] initWithIdentifier:identifier] autorelease];
	
	[toolbar setAllowsUserCustomization:YES];
	[toolbar setDisplayMode:NSToolbarDisplayModeIconAndLabel];
	[toolbar setSizeMode:NSToolbarSizeModeRegular];
	[toolbar setDelegate:self];
	
#ifndef RELEASE
	[toolbar setAutosavesConfiguration:YES];
#endif
	
	[[controller window] setToolbar:toolbar];
}

- (BOOL)hasUndoManager {
	return NO;
}
#pragma mark Reading and Writing
- (BOOL)readFromFileWrapper:(NSFileWrapper *)fileWrapper ofType:(NSString *)typeName error:(NSError **)outError {
	// we should only ever be asked to open project file types
#ifdef DEBUG
	NSAssert([typeName isEqualToString:kWCProjectUTI], @"passed document type other than project to WCProject!");
#endif
	
	NSDictionary *wrapperDict = [fileWrapper fileWrappers];
	NSFileWrapper *dataWrapper = [wrapperDict objectForKey:kWCProjectDataFileName];
	
	// the data file was moved
	if (!dataWrapper) {
		if (outError) {
			NSError *error = [NSError errorWithDomain:kWCProjectErrorDomain code:kWCProjectDataFileMovedErrorCode userInfo:nil];
			
			*outError = error;
		}
		return NO;
	}
	
	NSData *data = [dataWrapper regularFileContents];
	NSDictionary *dict = [NSKeyedUnarchiver unarchiveObjectWithData:data];
	
	// couldn't create dict from wrapper data or it's in the wrong format
	if (!dict) {
		if (outError) {
			NSError *error = [NSError errorWithDomain:kWCProjectErrorDomain code:kWCProjectDataFileIncorrectFormatErrorCode userInfo:nil];
			
			*outError = error;
		}
		return NO;
	}
	
	CGFloat version = [[dict objectForKey:kWCProjectVersionKey] floatValue];
	
	// older version that we can't read
	if (version < kWCProjectVersion) {
		if (outError) {
			NSError *error = [NSError errorWithDomain:kWCProjectErrorDomain code:kWCProjectDataFileOldVersionErrorCode userInfo:nil];
			
			*outError = error;
		}
		return NO;
	}
	
	WCProjectFile *projectFile = [dict objectForKey:kWCProjectFileKey];
	
	_projectFile = [projectFile retain];
	[_projectFile setProject:self];
	
	_buildTargets = [[NSMutableArray alloc] init];
	
	[_buildTargets addObjectsFromArray:[dict objectForKey:kWCProjectBuildTargetsKey]];
	
	for (WCBuildTarget *bt in _buildTargets)
		[bt setProject:self];
	
	for (WCFile *file in [self textFiles])
		[[file symbolScanner] scanSymbols];
	
	NSString *settingsFileName = [NSUserName() stringByAppendingPathExtension:@"wcodesettings"];
	NSFileWrapper *settingsWrapper = [wrapperDict objectForKey:settingsFileName];
	
	if (settingsWrapper) {
		NSData *settingsData = [settingsWrapper regularFileContents];
		
		if (settingsData) {
			NSMutableDictionary *settingsDict = [NSPropertyListSerialization propertyListWithData:settingsData options:NSPropertyListMutableContainers format:NULL error:NULL];
			
			_projectSettings = [settingsDict retain];
		}
	}
	else
		_projectSettings = [[NSMutableDictionary alloc] init];
		
	return YES;
}

- (NSFileWrapper *)fileWrapperOfType:(NSString *)typeName error:(NSError **)outError {
	return [[WCGeneralPerformer sharedPerformer] fileWrapperForProject:self error:outError];	
}
#pragma mark IBActions
- (void)saveDocument:(id)sender {
	[self saveProjectFile];
	
	WCFile *file = [[self selectedFileViewController] file];
	
	if (!file)
		return;
	
	NSError *error = nil;
	
	if (![file saveFile:&error] && error)
		[self presentError:error];
}

#pragma mark -
#pragma mark *** Protocol Overrides ***
#pragma mark NSToolbarDelegate

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar {
	if ([[toolbar identifier] isEqualToString:@"WCProjectToolbarIdentifier"])
		return [NSArray arrayWithObjects:kWCProjectToolbarBuildTargetPopUpButtonItemIdentifier,kWCProjectToolbarBuildItemIdentifier,kWCProjectToolbarBuildAndRunItemIdentifier,kWCProjectToolbarBuildAndDebugItemIdentifer,NSToolbarSeparatorItemIdentifier,NSToolbarFlexibleSpaceItemIdentifier,nil];
	return nil;
}

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar {
	if ([[toolbar identifier] isEqualToString:@"WCProjectToolbarIdentifier"])
		return [NSArray arrayWithObjects:kWCProjectToolbarBuildTargetPopUpButtonItemIdentifier,NSToolbarFlexibleSpaceItemIdentifier,kWCProjectToolbarBuildItemIdentifier,kWCProjectToolbarBuildAndRunItemIdentifier,kWCProjectToolbarBuildAndDebugItemIdentifer,NSToolbarFlexibleSpaceItemIdentifier,nil];
	return nil;
}

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag {
	NSToolbarItem *item = [[[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier] autorelease];
	
	if ([itemIdentifier isEqualToString:kWCProjectToolbarBuildItemIdentifier]) {
		[item setLabel:NSLocalizedString(@"Build", @"build toolbar item label")];
		[item setPaletteLabel:[item label]];
		[item setImage:[NSImage imageNamed:@"Building"]];
		[item setAction:@selector(build:)];
	}
	else if ([itemIdentifier isEqualToString:kWCProjectToolbarBuildAndRunItemIdentifier]) {
		[item setLabel:NSLocalizedString(@"Build and Run", @"build and run toolbar item label")];
		[item setPaletteLabel:[item label]];
		[item setImage:[NSImage imageNamed:@"BuildAndRun32x32"]];
		//[item setAction:@selector(build:)];
	}
	else if ([itemIdentifier isEqualToString:kWCProjectToolbarBuildAndDebugItemIdentifer]) {
		[item setLabel:NSLocalizedString(@"Build and Debug", @"build and debug toolbar item label")];
		[item setPaletteLabel:[item label]];
		[item setImage:[NSImage imageNamed:@"BuildAndDebug32x32"]];
		//[item setAction:@selector(build:)];
	}
	else if ([itemIdentifier isEqualToString:kWCProjectToolbarBuildTargetPopUpButtonItemIdentifier]) {
		[item setLabel:NSLocalizedString(@"Build Target", @"build target pop up button toolbar item label")];
		[item setPaletteLabel:[item label]];
		
		WCProjectBuildTargetPopUpButton *view = [[[WCProjectBuildTargetPopUpButton alloc] initWithProject:self] autorelease];
												 
		[item setView:view];
		[item setMinSize:NSMakeSize(150.0, NSHeight([view frame]))];
		[item setMaxSize:NSMakeSize(NSWidth([view frame]), NSHeight([view frame]))];
	}
	return item;
}

#pragma mark NSOpenSavePanelDelegate
- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
	return (![[self absoluteFilePaths] containsObject:[url path]]);
}
#pragma mark NSSplitViewDelegate
- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)view {
	if ([[splitView subviews] firstObject] == view)
		return NO;
	return YES;
}

#define kWCProjectSplitViewLeftMin 150.0
#define kWCProjectSplitViewRightMin 300.0

- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMaximumPosition ofSubviewAt:(NSInteger)dividerIndex {
	if (dividerIndex == 0)
		return proposedMaximumPosition - kWCProjectSplitViewRightMin;
	return proposedMaximumPosition;
}

- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMinimumPosition ofSubviewAt:(NSInteger)dividerIndex {
	if (dividerIndex == 0)
		return proposedMinimumPosition + kWCProjectSplitViewLeftMin;
	return proposedMinimumPosition;
}

- (NSRect)splitView:(NSSplitView *)splitView additionalEffectiveRectOfDividerAtIndex:(NSInteger)dividerIndex {
	return [splitView convertRectToBase:[[(id)[self currentViewController] splitterHandleImageView] frame]];
}

#pragma mark NSUserInterfaceValidations
- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
	if ([item action] == @selector(addFilesToProject:) &&
		[(id <NSObject>)item isKindOfClass:[NSMenuItem class]]) {
		
		[(NSMenuItem *)item setTitle:[NSString stringWithFormat:NSLocalizedString(@"Add Files to \"%@\"\u2026", @"project add files to project menu item title"),[self displayName]]];
		
		return YES;
	}
	else if ([item action] == @selector(group:)) {
		for (WCFile *file in [(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] selectedRepresentedObjects]) {
			if ([file isKindOfClass:[WCProjectFile class]])
				return NO;
		}
		return YES;
	}
	else if ([item action] == @selector(ungroup:)) {
		for (WCFile *file in [(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] selectedRepresentedObjects]) {
			if ([file isKindOfClass:[WCProjectFile class]] ||
				![file countOfChildNodes])
				return NO;
		}
		return YES;
	}
	else if ([item action] == @selector(delete:)) {
		for (WCFile *file in [(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] selectedRepresentedObjects])
			if ([file isKindOfClass:[WCProjectFile class]])
				return NO;
		return YES;
	}
	return [super validateUserInterfaceItem:item];
}
#pragma mark NSWindowDelegate
- (BOOL)windowShouldClose:(id)sender {
	if (![[self unsavedTextFiles] count])
		return YES;
	
	return ([WCUnsavedFilesWindowController runModalForProject:self] == NSOKButton);
}

- (void)windowWillClose:(NSNotification *)note {
	// save the project file before removing the tabs
	[self saveProjectFile];
	
	// otherwise crashes ensue, PSMTabBarControl retains things too long
	for (NSTabViewItem *item in [[[self tabBarControl] tabView] tabViewItems])
		[[[self tabBarControl] tabView] removeTabViewItem:item];
}
#pragma mark PSMTabBarControlDelegate
- (BOOL)tabView:(NSTabView *)tabView shouldCloseTabViewItem:(NSTabViewItem *)tabViewItem; {
	if ([[self openFiles] countForObject:[tabViewItem identifier]] > 1 || ![[tabViewItem identifier] hasUnsavedChanges])
		return YES;
	
	NSAlert *alert = [NSAlert alertWithMessageText:[NSString stringWithFormat:NSLocalizedString(@"Do you want to save the changes you made in the document \"%@\"?", @"unsaved file alert message text"),[[tabViewItem identifier] name]] defaultButton:NSLocalizedString(@"Save", @"Save") alternateButton:NSLocalizedString(@"Don't Save", @"Don't Save") otherButton:NS_LOCALIZED_STRING_CANCEL informativeTextWithFormat:NSLocalizedString(@"Your changes will be lost if you don't save them.", @"unsaved file alert informative text")];
	
	[alert setAlertStyle:NSCriticalAlertStyle];
	
	[alert beginSheetModalForWindow:[[tabViewItem tabView] window] completionHandler:^(NSAlert *mAlert, NSInteger result) {
		if (result == NSAlertOtherReturn)
			return;
		else if (result == NSAlertDefaultReturn)
			[[tabViewItem identifier] saveFile:NULL];
		else {
			[[tabViewItem identifier] resetFile:NULL];
		}
		
		[[tabViewItem tabView] removeTabViewItem:tabViewItem];
	}];
	
	return NO;
}
- (void)tabView:(NSTabView *)tabView didCloseTabViewItem:(NSTabViewItem *)tabViewItem; {
	[self removeFileViewControllerForFile:[tabViewItem identifier]];
}
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem {
	WCFileViewController *controller = [[[self filesToFileViewControllers] objectForKey:[tabViewItem identifier]] anyObject];
	
	[[[controller textView] syntaxHighlighter] performSyntaxHighlighting];
}
#pragma mark -
#pragma mark *** Public Methods ***
- (NSArray *)symbolsForSymbolName:(NSString *)name; {
	NSMutableArray *retval = [NSMutableArray array];
	
	for (WCFile *file in [self textFiles]) {
		[retval addObjectsFromArray:[[file symbolScanner] symbolsForSymbolName:name]];
	}
	
	return [[retval copy] autorelease];
}
- (void)jumpToBuildMessage:(WCBuildMessage *)message; {
	WCFileViewController *controller = [self addFileViewControllerForFile:[message file]];
	WCTextView *textView = [controller textView];
	NSRange range = NSMakeRange([[message file] lineStartForBuildMessage:message], 0);
	
	[textView setSelectedRangeSafely:range scrollRangeToVisible:YES];
}
- (void)jumpToSymbol:(WCSymbol *)symbol; {
	WCFileViewController *controller = [self addFileViewControllerForFile:[symbol file]];
	WCTextView *textView = [controller textView];
	NSRange range = [symbol symbolRange];
	
	[textView setSelectedRangeSafely:range scrollRangeToVisible:YES];
}
- (void)jumpToFindInProjectResult:(WCFindInProjectResult *)findResult; {
	WCFileViewController *controller = [self addFileViewControllerForFile:[findResult file]];
	WCTextView *textView = [controller textView];
	NSRange range = [findResult findRange];
	
	[textView setSelectedRangeSafely:range scrollRangeToVisible:YES];
}

- (void)saveProjectFile; {
	[self _updateProjectSettings];
	[super saveDocument:nil];
}
#pragma mark NSKeyValueCoding
- (NSUInteger)countOfBuildTargets {
	return [_buildTargets count];
}
- (id)objectInBuildTargetsAtIndex:(NSUInteger)index {
	return [_buildTargets objectAtIndex:index];
}
- (void)insertObject:(id)object inBuildTargetsAtIndex:(NSUInteger)index; {	
	[_buildTargets insertObject:object atIndex:index];
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCProjectNumberOfBuildTargetsDidChangeNotification object:self];
}
- (void)removeObjectFromChildNodesAtIndex:(NSUInteger)index; {	
	[_buildTargets removeObjectAtIndex:index];
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCProjectNumberOfBuildTargetsDidChangeNotification object:self];
}

- (NSUInteger)countOfBuildMessages {
	return [_buildMessages count];
}
- (id)objectInBuildMessagesAtIndex:(NSUInteger)index {
	return [_buildMessages objectAtIndex:index];
}
- (void)insertObject:(id)object inBuildMessagesAtIndex:(NSUInteger)index; {
	if (!_buildMessages)
		_buildMessages = [[NSMutableArray alloc] init];
	[_buildMessages insertObject:object atIndex:index];
}
- (void)removeObjectFromBuildMessagesAtIndex:(NSUInteger)index; {	
	[_buildMessages removeObjectAtIndex:index];
}
- (void)removeAllBuildMessages; {
	[[self mutableBuildMessages] removeAllObjects];
	for (WCFile *file in [self textFiles])
		[file removeAllBuildMessages];
}
#pragma mark Tabs
- (WCFileViewController *)addFileViewControllerForFile:(WCFile *)file; {
	WCFileViewController *controller = [self selectFileViewControllerForFile:file];
	
	if (controller)
		return controller;
	
	controller = [WCFileViewController fileViewControllerWithFile:file];
	
#ifdef DEBUG
	NSAssert(controller != nil, @"file view controller was nil!");
	NSAssert(file != nil, @"cannot add a tab without a file identifier!");
#endif
	
	NSTabViewItem *item = [[[NSTabViewItem alloc] initWithIdentifier:file] autorelease];
	[item setLabel:[file name]];
	[item setView:[controller view]];
	
	[[_tabBarControl tabView] addTabViewItem:item];
	[[_tabBarControl tabView] selectTabViewItem:item];
	
	NSMutableSet *controllers = [[self filesToFileViewControllers] objectForKey:file];
	
	if (!controllers) {
		controllers = [[[NSMutableSet alloc] initWithCapacity:1] autorelease];
		[[self filesToFileViewControllers] setObject:controllers forKey:file];
	}
	
	[controllers addObject:controller];
	[[self openFiles] addObject:file];
	
	return controller;
}

- (WCFileViewController *)selectFileViewControllerForFile:(WCFile *)file; {
	for (NSTabViewItem *item in [[[self tabBarControl] tabView] tabViewItems]) {
		if ([item identifier] == file && [[[[self tabBarControl] tabView] selectedTabViewItem] identifier] != [item identifier]) {
			[[[self tabBarControl] tabView] selectTabViewItem:item];
			return [[[self filesToFileViewControllers] objectForKey:file] anyObject];
		}
	}
	return nil;
}

- (void)removeFileViewControllerForFile:(WCFile *)file; {
#ifdef DEBUG
	NSAssert(file != nil, @"cannot remove a tab without a file identifier!");
#endif
	NSMutableSet *controllers = [[self filesToFileViewControllers] objectForKey:file];
	WCFileViewController *controllerToRemove = [controllers anyObject];
	
	/*
	for (WCFileViewController *controller in controllers) {
		if ([[controller view] superview] == [_tabBarControl tabView]) {
			controllerToRemove = controller;
			break;
		}
	}
	 */
	
#ifdef DEBUG
    NSAssert(controllerToRemove != nil, @"could not find a controller to remove!");
#endif
	
	if ([[self openFiles] countForObject:file] == 1)
		[[file undoManager] removeAllActions];
	
	[[file textStorage] removeLayoutManager:[[controllerToRemove textView] layoutManager]];
	[controllers removeObject:controllerToRemove];
	[[self openFiles] removeObject:file];
}
#pragma mark Accessors
@synthesize projectFile=_projectFile;
@synthesize buildTargets=_buildTargets;
@dynamic mutableBuildTargets;
- (NSMutableArray *)mutableBuildTargets {
	return [self mutableArrayValueForKey:@"buildTargets"];
}
@dynamic filesToFileViewControllers;
- (NSMapTable *)filesToFileViewControllers {
	if (!_filesToFileViewControllers)
		_filesToFileViewControllers = [[NSMapTable mapTableWithStrongToStrongObjects] retain];
	
	return _filesToFileViewControllers;
}
@dynamic textFiles;
- (NSArray *)textFiles {
	NSMutableArray *retval = [NSMutableArray array];
	for (WCFile *file in [[self projectFile] descendantLeafNodes]) {
		NSString *UTI = [file UTI];
		
		if ([UTI isEqualToString:kWCFileAssemblyUTI] ||
			[UTI isEqualToString:kWCFileIncludeUTI] ||
			[UTI isEqualToString:kWCFilePanicCodaImportedUTI])
			[retval addObject:file];
	}
	return [[retval copy] autorelease];
}
@dynamic symbols;
- (NSArray *)symbols {
	NSMutableArray *retval = [NSMutableArray array];
	for (WCFile *file in [self textFiles])
		[retval addObjectsFromArray:[[file symbolScanner] symbols]];
	return [[retval copy] autorelease];
}
@dynamic labelSymbols;
- (NSArray *)labelSymbols {
	NSMutableArray *retval = [NSMutableArray array];
	for (WCFile *file in [self textFiles]) {
		[retval addObjectsFromArray:[[[file symbolScanner] labelNamesToSymbols] allValues]];
	}
	return [[retval copy] autorelease];
}
@dynamic equateSymbols;
- (NSArray *)equateSymbols {
	NSMutableArray *retval = [NSMutableArray array];
	for (WCFile *file in [self textFiles]) {
		[retval addObjectsFromArray:[[[file symbolScanner] equateNamesToSymbols] allValues]];
	}
	return [[retval copy] autorelease];
}
@dynamic defineSymbols;
- (NSArray *)defineSymbols {
	NSMutableArray *retval = [NSMutableArray array];
	for (WCFile *file in [self textFiles]) {
		[retval addObjectsFromArray:[[[file symbolScanner] defineNamesToSymbols] allValues]];
	}
	return [[retval copy] autorelease];
}
@dynamic macroSymbols;
- (NSArray *)macroSymbols {
	NSMutableArray *retval = [NSMutableArray array];
	for (WCFile *file in [self textFiles]) {
		[retval addObjectsFromArray:[[[file symbolScanner] macroNamesToSymbols] allValues]];
	}
	return [[retval copy] autorelease];
}
@dynamic labelNamesToSymbols;
- (NSDictionary *)labelNamesToSymbols {
	NSMutableDictionary *retval = [NSMutableDictionary dictionary];
	for (WCFile *file in [self textFiles]) {
		[retval addEntriesFromDictionary:[[file symbolScanner] labelNamesToSymbols]];
	}
	return [[retval copy] autorelease];
}
@dynamic equateNamesToSymbols;
- (NSDictionary *)equateNamesToSymbols {
	NSMutableDictionary *retval = [NSMutableDictionary dictionary];
	for (WCFile *file in [self textFiles]) {
		[retval addEntriesFromDictionary:[[file symbolScanner] equateNamesToSymbols]];
	}
	return [[retval copy] autorelease];
}
@dynamic defineNamesToSymbols;
- (NSDictionary *)defineNamesToSymbols {
	NSMutableDictionary *retval = [NSMutableDictionary dictionary];
	for (WCFile *file in [self textFiles]) {
		[retval addEntriesFromDictionary:[[file symbolScanner] defineNamesToSymbols]];
	}
	return [[retval copy] autorelease];
}
@dynamic macroNamesToSymbols;
- (NSDictionary *)macroNamesToSymbols {
	NSMutableDictionary *retval = [NSMutableDictionary dictionary];
	for (WCFile *file in [self textFiles]) {
		[retval addEntriesFromDictionary:[[file symbolScanner] macroNamesToSymbols]];
	}
	return [[retval copy] autorelease];
}
@dynamic selectedFileViewController;
- (WCFileViewController *)selectedFileViewController {
	return [[[self filesToFileViewControllers] objectForKey:[[[_tabBarControl tabView] selectedTabViewItem] identifier]] anyObject];
}
@dynamic activeBuildTarget;
- (WCBuildTarget *)activeBuildTarget {
	WCBuildTarget *retval = nil;
	for (WCBuildTarget *bt in [self buildTargets]) {
		if ([bt isActive]) {
			retval = bt;
			break;
		}
	}
	
	if (!retval && [[self buildTargets] count]) {
		WCBuildTarget *bt = [[self buildTargets] lastObject];
		[bt setIsActive:YES];
		retval = bt;
	}
	return retval;
}
- (void)setActiveBuildTarget:(WCBuildTarget *)target {
	if ([self activeBuildTarget] == target)
		return;
	
	for (WCBuildTarget *bt in [self buildTargets])
		[bt setIsActive:NO];
	
	[target setIsActive:YES];

	[[NSNotificationCenter defaultCenter] postNotificationName:kWCProjectActiveBuildTargetDidChangeNotification object:self];
}
@dynamic openFiles;
- (NSCountedSet *)openFiles {
	if (!_openFiles)
		_openFiles = [[NSCountedSet alloc] init];
	return _openFiles;
}
@dynamic unsavedTextFiles;
- (NSArray *)unsavedTextFiles {
	NSMutableArray *retval = [NSMutableArray array];
	for (WCFile *file in [self openFiles]) {
		if ([file hasUnsavedChanges])
			[retval addObject:file];
	}
	return [[retval copy] autorelease];
}
@synthesize projectSettings=_projectSettings;
@synthesize isBuilding=_isBuilding;
@dynamic currentViewController;
- (WCProjectNavigationViewController *)currentViewController {
	return _currentViewController;
}
- (void)setCurrentViewController:(WCProjectNavigationViewController *)controller {
	if (_currentViewController == controller) {
		return;
	}
	
	if (!_currentViewController) {
		[[controller view] setFrameSize:[_swapView frame].size];
		[_swapView addSubview:[controller view]];
	}
	else {
		[[controller view] setFrameSize:[[_currentViewController view] frame].size];
		[_swapView replaceSubview:[_currentViewController view] with:[controller view]];
	}
	
	_currentViewController = controller;
	
	// to counteract the BWAnchoredButtonBar, which sets itself as the delegate
	[_splitView setDelegate:self];
	[[[_currentViewController view] window] makeFirstResponder:[_currentViewController firstResponder]];	
}
@dynamic projectFilesOutlineViewController;
- (WCProjectFilesOutlineViewController *)projectFilesOutlineViewController {
	if (!_projectFilesOutlineViewController)
		_projectFilesOutlineViewController = [[WCProjectFilesOutlineViewController projectNavigationViewControllerWithProject:self] retain];
	return _projectFilesOutlineViewController;
}
@dynamic buildMessagesViewController;
- (WCBuildMessagesViewController *)buildMessagesViewController {
	if (!_buildMessagesViewController)
		_buildMessagesViewController = [[WCBuildMessagesViewController projectNavigationViewControllerWithProject:self] retain];
	
	return _buildMessagesViewController;
}
@synthesize buildMessages=_buildMessages;
@dynamic mutableBuildMessages;
- (NSMutableArray *)mutableBuildMessages {
	if (!_buildMessages)
		_buildMessages = [[NSMutableArray alloc] init];
	return [self mutableArrayValueForKey:@"buildMessages"];
}
@dynamic symbolsViewController;
- (WCSymbolsViewController *)symbolsViewController {
	if (!_symbolsViewController)
		_symbolsViewController = [[WCSymbolsViewController projectNavigationViewControllerWithProject:self] retain];
	return _symbolsViewController;
}
@dynamic findInProjectViewController;
- (WCFindInProjectViewController *)findInProjectViewController {
	if (!_findInProjectViewController)
		_findInProjectViewController = [[WCFindInProjectViewController projectNavigationViewControllerWithProject:self] retain];
	return _findInProjectViewController;
}
@dynamic findInProjectViewControllerDontCreate;
- (WCFindInProjectViewController *)findInProjectViewControllerDontCreate {
	return _findInProjectViewController;
}
@synthesize tabBarControl=_tabBarControl;
@synthesize ignoreUnsavedChanges=_ignoreUnsavedChanges;
@synthesize ignoreUnsavedChangesForProjectClose=_ignoreUnsavedChangesForProjectClose;
#pragma mark IBActions
- (IBAction)addFilesToProject:(id)sender; {
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	
	[panel setAllowsMultipleSelection:YES];
	[panel setCanChooseDirectories:YES];
	[panel setPrompt:NS_LOCALIZED_STRING_ADD];
	[panel setAccessoryView:[[[[WCAddFilesToProjectViewController alloc] init] autorelease] view]];
	[panel setDelegate:self];
	
	[self setAbsoluteFilePaths:[NSSet setWithArray:[[[self projectFile] descendantNodes] valueForKeyPath:@"absolutePath"]]];
	
	[panel beginSheetModalForWindow:[self windowForSheet] completionHandler:^(NSInteger result) {
		if (result == NSFileHandlingPanelCancelButton)
			return;
		
		WCFile *file = [(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] selectedRepresentedObject];
		NSUInteger index = 0;
		
		if (![file isDirectory]) {
			index = [[[file parentNode] childNodes] indexOfObject:file];
			file = [file parentNode];
		}
			
		[[WCGeneralPerformer sharedPerformer] addFileURLs:[panel URLs] toFile:file atIndex:index];
		
		// update our cached file paths
		[self setAbsoluteFilePaths:[NSSet setWithArray:[[[self projectFile] descendantNodes] valueForKeyPath:@"absolutePath"]]];
		
		[[NSNotificationCenter defaultCenter] postNotificationName:kWCProjectNumberOfFilesDidChangeNotification object:self];		
	}];
}

- (IBAction)newFile:(id)sender; {
	[WCNewFileWindowController presentNewFileSheetForProject:self];
}

- (IBAction)build:(id)sender; {
	if ([self isBuilding]) {
		NSBeep();
		return;
	}
	
	// check to see if the project has any unsaved files
	NSMutableArray *ufiles = [NSMutableArray array];
	for (WCFile *file in [self openFiles]) {
		if ([file hasUnsavedChanges])
			[ufiles addObject:file];
	}
	
	if ([ufiles count]) {
		WCPreferencesBuildingForUnsavedFiles choice = [[NSUserDefaults standardUserDefaults] unsignedIntegerForKey:kWCPreferencesBuildingForUnsavedFilesKey];
		
		switch (choice) {
			case WCPreferencesBuildingForUnsavedFilesAlwaysSave:
				for (WCFile *file in ufiles)
					[file saveFile:NULL];
				break;
			case WCPreferencesBuildingForUnsavedFilesPromptBeforeSaving:
				break;
			case WCPreferencesBuildingForUnsavedFilesNeverSave:
			default:
				break;
		}
	}
	
	WCBuildTarget *bt = [self activeBuildTarget];
	
	if (![bt inputFileAlias]) {
		NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"No Input File", @"active build target has no input file alert message text") defaultButton:NSLocalizedString(@"Edit\u2026", @"Edit\u2026") alternateButton:NS_LOCALIZED_STRING_CANCEL otherButton:nil informativeTextWithFormat:NSLocalizedString(@"The active build target \"%@\" does not have an input file assigned to it. Would you like to edit the target now?", @"active build target has no input file alert informative text"),[bt name]];
		
		[alert beginSheetModalForWindow:[self windowForSheet] modalDelegate:self didEndSelector:@selector(_activeBuildTargetHasNoInputFileAlertDidEnd:code:info:) contextInfo:NULL];
		return;
	}
	
	NSString *outputDirectory = nil;
	WCPreferencesBuildingLocation choice = [[NSUserDefaults standardUserDefaults] unsignedIntegerForKey:kWCPreferencesBuildingLocationKey];
	
	switch (choice) {
		case WCPreferencesBuildingLocationProjectFolder: {
			NSURL *projectDirectory = [[self fileURL] URLByDeletingLastPathComponent];
			projectDirectory = [[projectDirectory URLByAppendingPathComponent:NSLocalizedString(@"build", @"build")] URLByAppendingPathComponent:[bt name]];
			
			if (![[NSFileManager defaultManager] createDirectoryAtPath:[projectDirectory path] withIntermediateDirectories:YES attributes:nil error:NULL]) {
				if (![projectDirectory checkResourceIsReachableAndReturnError:NULL]) {
					NSLog(@"could not create output directory and it does not already exist");
					return;
				}
			}
			
			outputDirectory = [projectDirectory path];
		}
			break;
		case WCPreferencesBuildingLocationCustom:
			break;
		default:
			break;
	}
	
#ifdef DEBUG
    NSAssert(outputDirectory != nil, @"output directory cannot be nil!");
#endif
	
	NSString *inputFile = [[[bt inputFileAlias] URL] path];
	NSString *outputName = [bt outputName];
	NSString *outputExtension = [bt outputExtension];
	
#ifdef DEBUG
    NSAssert(outputName != nil && outputExtension != nil, @"outputName and outputExtension cannot be nil!");
#endif
	
	NSString *outputFile = [outputDirectory stringByAppendingPathComponent:[outputName stringByAppendingPathExtension:outputExtension]];
	
	[self removeAllBuildMessages];
	
	NSMutableArray *args = [NSMutableArray array];
	
	if ([bt generateCodeListing])
		[args addObject:@"-T"];
	if ([bt generateLabelFile])
		[args addObject:@"-L"];
	if ([bt symbolsAreCaseSensitive])
		[args addObject:@"-A"];
	if ([[bt includeDirectories] count])
		for (WCIncludeDirectory *idir in [bt includeDirectories])
			[args addObject:[NSString stringWithFormat:@"-I%@",[idir absolutePathForDisplay]]];
	if ([[bt defines] count])
		for (WCBuildDefine *def in [bt defines])
			[args addObject:[def processedDefine]];
	
	[args addObject:inputFile];
	[args addObject:outputFile];
	
	[_buildTask release];
	_buildTask = [[NSTask alloc] init];
	
	[_buildTask setLaunchPath:[[NSBundle mainBundle] pathForResource:@"spasm" ofType:@""]];
	[_buildTask setCurrentDirectoryPath:[[[self fileURL] URLByDeletingLastPathComponent] path]];
	[_buildTask setStandardOutput:[NSPipe pipe]];
	[_buildTask setStandardError:[_buildTask standardOutput]];
	[_buildTask setArguments:args];
	
#ifdef DEBUG
    NSLog(@"%@",[_buildTask arguments]);
#endif
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_readDataFromBuildTask:) name:NSFileHandleReadCompletionNotification object:[[_buildTask standardOutput] fileHandleForReading]];
	
	[[[_buildTask standardOutput] fileHandleForReading] readInBackgroundAndNotify];
	
	[self setIsBuilding:YES];
	
	@try {
		[_buildTask launch];
	}
	@catch (NSException *exception) {
		NSLog(@"exception was thrown when spasm failed");
	}
}

- (IBAction)editBuildTargets:(id)sender; {
	[WCBuildTargetInfoSheetController presentBuildTargetInfoSheetControllerForProject:self];
}

- (IBAction)newGroup:(id)sender; {
	WCFile *file = [(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] selectedRepresentedObject];
	WCFile *group = [WCFile fileWithURL:[file directoryURL] name:NSLocalizedString(@"New Group", @"New Group")];
	
	if ([file isDirectory])
		[[file mutableChildNodes] insertObject:group atIndex:0];
	else
		[[[file parentNode] mutableChildNodes] insertObject:group atIndex:[[[file parentNode] childNodes] indexOfObject:file]];
	
	[(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] setSelectedRepresentedObject:group];
	[[[self projectFilesOutlineViewController] outlineView] editColumn:0 row:[[[self projectFilesOutlineViewController] outlineView] selectedRow] withEvent:nil select:YES];
}

- (IBAction)group:(id)sender; {
	NSArray *files = [(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] selectedRepresentedObjects];
	WCFile *firstFile = [files firstObject];
	WCFile *group = [WCFile fileWithURL:[firstFile directoryURL] name:NSLocalizedString(@"New Group", @"New Group")];
	
	[[[firstFile parentNode] mutableChildNodes] insertObject:group atIndex:[[[firstFile parentNode] childNodes] indexOfObject:firstFile]];
	
	[(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] moveNodes:[(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] treeNodesForRepresentedObjects:files] toIndexPath:[[(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] indexPathForRepresentedObject:group] indexPathByAddingIndex:0]];
	
	[(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] setSelectedRepresentedObject:group];
	[[[self projectFilesOutlineViewController] outlineView] collapseItem:[[[self projectFilesOutlineViewController] outlineView] itemAtRow:[[[self projectFilesOutlineViewController] outlineView] selectedRow]]];
	[[[self projectFilesOutlineViewController] outlineView] editColumn:0 row:[[[self projectFilesOutlineViewController] outlineView] selectedRow] withEvent:nil select:YES];
}

- (IBAction)ungroup:(id)sender; {
	NSArray *nodesToRemove = [(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] selectedNodes];
	//NSMutableArray *objectsToSelect = [nodesToRemove valueForKeyPath:@"@distinctUnionOfArrays.childNodes"];
	NSMutableArray *objectsToSelect = [NSMutableArray array];
	
	for (NSTreeNode *node in nodesToRemove)
		[objectsToSelect addObjectsFromArray:[node childNodes]];
	
	[(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] moveNodes:objectsToSelect toIndexPath:[[nodesToRemove firstObject] indexPath]];
	
	for (NSTreeNode *node in nodesToRemove)
		[[[node parentNode] mutableChildNodes] removeObject:node];
	
	[(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] setSelectedTreeNodes:objectsToSelect];
}

- (IBAction)delete:(id)sender {
	// deletes are only allowed in the files outline view, for now at least
	if ([self currentViewController] != [self projectFilesOutlineViewController]) {
		NSBeep();
		return;
	}
	
	NSArray *files = [[self currentViewController] selectedObjects];
	BOOL canDeleteWithoutAlert = YES;
	
	// we allow deletes of groups that are empty or only contain other empty groups
	for (WCFile *file in files) {
		if (![file isDirectory]) {
			canDeleteWithoutAlert = NO;
			break;
		}
	}
	
	// delete the groups without confirming
	if (canDeleteWithoutAlert) {
		for (WCFile *file in files)
			[[[file parentNode] mutableChildNodes] removeObject:file];
		return;
	}
	
	// otherwise we have to confirm with the user first before proceeding
	NSAlert *alert = [NSAlert alertWithMessageText:([files count] == 1)?[NSString stringWithFormat:NSLocalizedString(@"Delete \"%@\"", @"delete one file alert message text"),[[files firstObject] name]]:[NSString stringWithFormat:NSLocalizedString(@"Delete %u Files", @"delete multiple files alert message text"),[files count]] defaultButton:([files count] == 1)?NSLocalizedString(@"Remove Reference Only", @"delete one file alert default button title"):NSLocalizedString(@"Remove References Only", @"delete multiple files alert default button title") alternateButton:NS_LOCALIZED_STRING_CANCEL otherButton:NS_LOCALIZED_STRING_DELETE informativeTextWithFormat:([files count] == 1)?[NSString stringWithFormat:NSLocalizedString(@"Do you want to delete \"%@\" or only remove the reference to it? This operation cannot be undone. Unsaved changes will be lost.", @"delete one file alert informative text"),[[files firstObject] name]]:[NSString stringWithFormat:NSLocalizedString(@"Do you want to delete %u files or only remove the references to them? This operation cannot be undone. Unsaved changes will be lost.", @"delete multiple files alert informative text"),[files count]]];
	
	[alert beginSheetModalForWindow:[self windowForSheet] modalDelegate:self didEndSelector:@selector(_deleteAlertDidEnd:code:info:) contextInfo:NULL];
}

- (IBAction)viewProject:(id)sender; {
	[self setCurrentViewController:[self projectFilesOutlineViewController]];
	[_navBarControl setSelectedIndex:[[_navBarControl selectors] indexOfObject:NSStringFromSelector(_cmd)]];
}
- (IBAction)viewBuildMessages:(id)sender; {
	[self setCurrentViewController:[self buildMessagesViewController]];
	[_navBarControl setSelectedIndex:[[_navBarControl selectors] indexOfObject:NSStringFromSelector(_cmd)]];
}
- (IBAction)viewSymbols:(id)sender; {
	[self setCurrentViewController:[self symbolsViewController]];
	[_navBarControl setSelectedIndex:[[_navBarControl selectors] indexOfObject:NSStringFromSelector(_cmd)]];
}
- (IBAction)viewSearch:(id)sender; {
	[self setCurrentViewController:[self findInProjectViewController]];
	[_navBarControl setSelectedIndex:[[_navBarControl selectors] indexOfObject:NSStringFromSelector(_cmd)]];
}
- (IBAction)changeProjectView:(WCProjectNavView *)sender; {
	[self performSelector:NSSelectorFromString([[sender selectors] objectAtIndex:[sender selectedIndex]]) withObject:nil];
}
#pragma mark -
#pragma mark *** Private Methods ***
- (void)_addBuildMessageForString:(NSString *)string; {
#ifdef DEBUG
    NSLog(@"%@",string);
#endif
	
	NSArray *parts = [string componentsSeparatedByString:@":"];
	
	if ([parts count] == 4) {
		
		WCFile *mFile = nil;
		for (WCFile *file in [self textFiles]) {
			if ([[file absolutePath] isEqualToString:[parts firstObject]]) {
				mFile = file;
				break;
			}
		}
		
		WCBuildMessage *mMessage = nil;
		for (WCBuildMessage *message in [self buildMessages]) {
			if ([message file] == mFile) {
				mMessage = message;
				break;
			}
		}
		
		NSUInteger line = (NSUInteger)[(NSString *)[parts objectAtIndex:1] integerValue];
		NSString *message = [[parts objectAtIndex:3] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
		WCBuildMessageType type = WCBuildMessageTypeFile;
		
		if ([[parts objectAtIndex:2] rangeOfString:@"error" options:NSLiteralSearch].length)
			type = WCBuildMessageTypeError;
		else if ([[parts objectAtIndex:2] rangeOfString:@"warning" options:NSLiteralSearch].length)
			type = WCBuildMessageTypeWarning;
		
		if (!mFile) {
			/*
			if ([[parts objectAtIndex:0] rangeOfString:@"Built-in"].length) {
				mMessage = [WCBuildMessage buildMessageWithMessage:[NSString stringWithFormat:@"%@:%u: %@",[parts objectAtIndex:0],(NSUInteger)line,originalMessage] ofType:type inFile:[self projectFile] atLineNumber:0];
				[[[self rootBuildMessage] mutableChildNodes] addObject:mMessage];
			}
			 */
			return;
		}
		
		if (!mMessage) {
			mMessage = [WCBuildMessage buildMessageWithMessage:nil ofType:WCBuildMessageTypeFile inFile:mFile atLineNumber:0];
			[[self mutableBuildMessages] addObject:mMessage];
		}
		
#ifdef DEBUG
		NSAssert(mFile != nil && mMessage != nil, @"matching file for build message not found!");
#endif
		
		WCBuildMessage *nMessage = [WCBuildMessage buildMessageWithMessage:message ofType:type inFile:mFile atLineNumber:line];
		
		[[mMessage mutableChildNodes] addObject:nMessage];
		
		if (type == WCBuildMessageTypeError)
			[mFile addErrorMessage:nMessage];
		else if (type == WCBuildMessageTypeWarning) {
			if ([[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesBuildingTreatWarningsAsErrorsKey])
				[mFile addErrorMessage:nMessage];
			else
				[mFile addWarningMessage:nMessage];
		}
	}
}

- (void)_deleteObjects:(NSArray *)objects deleteFiles:(BOOL)deleteFiles; {
	if ([[objects firstObject] isKindOfClass:[WCFile class]]) {
		if (deleteFiles) {
			// move the represented files to the trash can
			[[NSWorkspace sharedWorkspace] recycleURLs:[objects valueForKeyPath:@"URL"] completionHandler:nil];
		}
		
		// close any open tabs representing the files
		for (WCFile *file in objects) {
			for (NSTabViewItem *item in [[_tabBarControl tabView] tabViewItems]) {
				if ([item identifier] == file) {
					[[_tabBarControl tabView] removeTabViewItem:item];
					break;
				}
			}
			
			//[self removeFileViewControllerForFile:file];
		}
		
		for (WCFile *file in objects) {
			WCFindInProjectResult *fResultToRemove = nil;
			for (WCFindInProjectResult *fResult in [[self findInProjectViewControllerDontCreate] findResults]) {
				if ([fResult file] == file) {
					fResultToRemove = fResult;
					break;
				}
			}
			if (fResultToRemove)
				[[[self findInProjectViewControllerDontCreate] mutableFindResults] removeObject:fResultToRemove];
		}
		
		// remove the objects from our model
		for (WCFile *file in objects)
			[[[file parentNode] mutableChildNodes] removeObject:file];
		
		// update our cached file paths
		[self setAbsoluteFilePaths:[NSSet setWithArray:[[[self projectFile] descendantNodes] valueForKeyPath:@"absolutePath"]]];
		
		[[NSNotificationCenter defaultCenter] postNotificationName:kWCProjectNumberOfFilesDidChangeNotification object:self];
	}
}
#pragma mark Project Settings
- (void)_updateProjectSettings; {
	[[self projectSettings] setObject:[[[self projectFilesOutlineViewController] outlineView] expandedItemUUIDs] forKey:kWCProjectSettingsProjectFilesOutlineViewExpandedItemUUIDsKey];
	[[self projectSettings] setObject:[[self windowForSheet] stringWithSavedFrame] forKey:kWCProjectSettingsProjectWindowFrameKey];
	[[self projectSettings] setObject:[NSNumber numberWithDouble:[[[_splitView subviews] firstObject] frame].size.width] forKey:kWCProjectSettingsLeftVerticalSplitViewDividerPositionKey];
	[[self projectSettings] setObject:[[[[self tabBarControl] tabView] tabViewItems] valueForKeyPath:@"identifier.UUID"] forKey:kWCProjectSettingsOpenFileUUIDsKey];
	if ([[[self tabBarControl] tabView] numberOfTabViewItems])
		[[self projectSettings] setObject:[[[[self tabBarControl] tabView] selectedTabViewItem] valueForKeyPath:@"identifier.UUID"] forKey:kWCProjectSettingsSelectedFileUUIDKey];
	
}
- (void)_applyProjectSettings; {
	// expand the right items in our files outline view
	[[[self projectFilesOutlineViewController] outlineView] expandItemsWithUUIDs:[[self projectSettings] objectForKey:kWCProjectSettingsProjectFilesOutlineViewExpandedItemUUIDsKey]];
	
	// restore the project window's frame
	if (![[self projectSettings] objectForKey:kWCProjectSettingsProjectWindowFrameKey])
		[[self windowForSheet] center];
	else
		[[self windowForSheet] setFrameFromString:[[self projectSettings] objectForKey:kWCProjectSettingsProjectWindowFrameKey]];
	
	// restore the position of the left vertical split view divider
	if ([[self projectSettings] objectForKey:kWCProjectSettingsLeftVerticalSplitViewDividerPositionKey])
		[_splitView setPosition:[[[self projectSettings] objectForKey:kWCProjectSettingsLeftVerticalSplitViewDividerPositionKey] doubleValue] ofDividerAtIndex:0];
	
	// restore open files
	NSArray *tFiles = [self textFiles];
	for (NSString *UUID in [[self projectSettings] objectForKey:kWCProjectSettingsOpenFileUUIDsKey]) {
		for (WCFile *file in tFiles) {
			if ([[file UUID] isEqualToString:UUID]) {
				[self addFileViewControllerForFile:file];
				break;
			}
		}
	}
	
	// restore the selected file
	if ([[self projectSettings] objectForKey:kWCProjectSettingsSelectedFileUUIDKey]) {
		NSString *sUUID = [[self projectSettings] objectForKey:kWCProjectSettingsSelectedFileUUIDKey];
		
		for (WCFile *file in tFiles) {
			if ([[file UUID] isEqualToString:sUUID]) {
				[[[self tabBarControl] tabView] selectTabViewItemWithIdentifier:file];
				break;
			}
		}
	}
}
#pragma mark Accessors
@synthesize absoluteFilePaths=_cachedAbsoluteFilePaths;
#pragma mark IBActions
- (IBAction)_outlineViewDoubleClick:(id)sender; {
	NSTreeNode *node = [(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] selectedNode];
	WCFile *file = [node representedObject];
	
	if ([[self textFiles] containsObject:file])
		[self addFileViewControllerForFile:file];
	else if ([file isDirectory]) {
		if ([[[self projectFilesOutlineViewController] outlineView] isItemExpanded:node])
			[[[self projectFilesOutlineViewController] outlineView] collapseItem:node];
		else
			[[[self projectFilesOutlineViewController] outlineView] expandItem:node];
	}
}
- (IBAction)_buildMessagesOutlineViewDoubleAction:(id)sender {
	NSTreeNode *node = [(NSTreeController *)[[[self buildMessagesViewController] outlineView] dataSource] selectedNode];
	WCBuildMessage *message = [node representedObject];
	
	if ([message isLeaf])
		[self jumpToBuildMessage:message];
	else {
		if ([[[self buildMessagesViewController] outlineView] isItemExpanded:node])
			[[[self buildMessagesViewController] outlineView] collapseItem:node];
		else
			[[[self buildMessagesViewController] outlineView] expandItem:node];
	}
}
- (IBAction)_symbolsOutlineViewDoubleAction:(id)sender {
	WCSymbol *symbol = [[self symbolsViewController] selectedObject];
	
	if ([symbol isLeaf])
		[self jumpToSymbol:symbol];
	else {
		if ([[[self symbolsViewController] outlineView] isItemExpanded:symbol])
			[[[self symbolsViewController] outlineView] collapseItem:symbol];
		else
			[[[self symbolsViewController] outlineView] expandItem:symbol];
	}
}
- (IBAction)_findInProjectOutlineViewDoubleAction:(id)sender {
	WCFindInProjectResult *fResult = [[self findInProjectViewController] selectedObject];
	NSTreeNode *node = [[self findInProjectViewController] selectedNode];
	
	if ([fResult isLeaf])
		[self jumpToFindInProjectResult:fResult];
	else {
		if ([[[self findInProjectViewController] outlineView] isItemExpanded:node])
			[[[self findInProjectViewController] outlineView] collapseItem:node];
		else
			[[[self findInProjectViewController] outlineView] expandItem:node];
	}
}
#pragma mark Notifications
- (void)_readDataFromBuildTask:(NSNotification *)note {	
	NSData *data = [[note userInfo] objectForKey:NSFileHandleNotificationDataItem];
	
	if ([data length]) {
		NSString *message = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
		
		for (NSString *sMessage in [message componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]])
			[self _addBuildMessageForString:sMessage];
		
		[[note object] readInBackgroundAndNotify];
	}
	else {
		[[NSNotificationCenter defaultCenter] removeObserver:self name:NSFileHandleReadCompletionNotification object:[[_buildTask standardOutput] fileHandleForReading]];
		
		[_buildTask terminate];
		
		NSData *eData = nil;
		while ((eData = [[[_buildTask standardOutput] fileHandleForReading] availableData]) && [eData length]) {
			NSString *message = [[[NSString alloc] initWithData:eData encoding:NSUTF8StringEncoding] autorelease];
			
			for (NSString *sMessage in [message componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]])
				[self _addBuildMessageForString:sMessage];
		}
		
		if ([[self buildMessages] count]) {
			NSUInteger errors = 0;
			NSUInteger warnings = 0;
			for (WCBuildMessage *m in [self buildMessages]) {
				for (WCBuildMessage *mm in [m childNodes]) {
					if ([mm messageType] == WCBuildMessageTypeError)
						errors++;
					else if ([mm messageType] == WCBuildMessageTypeWarning)
						warnings++;
				}
			}
			
			if (errors && warnings) {
				if (!_errorBadge) {
					_errorBadge = [[CTBadge badgeWithColor:[NSColor redColor] labelColor:[NSColor whiteColor]] retain];
				}
				if (!_warningBadge) {
					_warningBadge = [[CTBadge badgeWithColor:[NSColor orangeColor] labelColor:[NSColor whiteColor]] retain];
				}
				
				NSImage *eBadge = [_errorBadge badgeOverlayImageForValue:errors insetX:0.0 y:0.0];
				NSImage *wBadge = [_warningBadge badgeOverlayImageForValue:warnings insetX:0.0 y:CTLargeBadgeSize];
				NSImage *appImage = [[_appIcon copy] autorelease];
				
				[appImage lockFocus];
				[eBadge drawInRect:NSMakeRect(0.0, 0.0, [eBadge size].width, [eBadge size].height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
				[wBadge drawInRect:NSMakeRect(0.0, 0.0, [wBadge size].width, [wBadge size].height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
				[appImage unlockFocus];
				 
				[NSApp setApplicationIconImage:appImage];
			}
			else if (errors) {
				if (!_errorBadge) {
					_errorBadge = [[CTBadge badgeWithColor:[NSColor redColor] labelColor:[NSColor whiteColor]] retain];

				}
				
				NSImage *eBadge = [_errorBadge badgeOverlayImageForValue:errors insetX:0.0 y:0.0];
				NSImage *appImage = [[_appIcon copy] autorelease];
				
				[appImage lockFocus];
				[eBadge drawInRect:NSMakeRect(0.0, 0.0, [eBadge size].width, [eBadge size].height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
				[appImage unlockFocus];
				
				[NSApp setApplicationIconImage:appImage];
			}
			else if (warnings) {
				if (!_warningBadge) {
					_warningBadge = [[CTBadge badgeWithColor:[NSColor orangeColor] labelColor:[NSColor whiteColor]] retain];
				}
				
				NSImage *wBadge = [_warningBadge badgeOverlayImageForValue:warnings insetX:0.0 y:0.0];
				NSImage *appImage = [[_appIcon copy] autorelease];
				
				[appImage lockFocus];
				[wBadge drawInRect:NSMakeRect(0.0, 0.0, [wBadge size].width, [wBadge size].height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
				[appImage unlockFocus];
				
				[NSApp setApplicationIconImage:appImage];
			}
			
			[self viewBuildMessages:nil];
			
			for (WCBuildMessage *bm in [self buildMessages]) {
				[[[self buildMessagesViewController] outlineView] expandItem:[(NSTreeController *)[[[self buildMessagesViewController] outlineView] dataSource] treeNodeForRepresentedObject:bm]];
			}
		}
		else {
			[NSApp setApplicationIconImage:_appIcon];
			
			//[[NSWorkspace sharedWorkspace] launchApplication:@"WabbitEmu"];
			//[[NSWorkspace sharedWorkspace] openFile:[[_buildTask arguments] lastObject] withApplication:@"WabbitEmu" andDeactivate:YES];
		}
		
		[self setIsBuilding:NO];
	}
}

- (void)_fileHasUnsavedChanges:(NSNotification *)note {
	WCFile *file = [note object];
	
	if ([file project] == self)
		[[[self projectFilesOutlineViewController] outlineView] setNeedsDisplayInRect:[[[self projectFilesOutlineViewController] outlineView] rectOfRow:[[[self projectFilesOutlineViewController] outlineView] rowForItem:[(NSTreeController *)[[[self projectFilesOutlineViewController] outlineView] dataSource] treeNodeForRepresentedObject:file]]]];
}

- (void)_fileNameDidChange:(NSNotification *)note {
	WCFile *file = [note object];
	
	if ([file project] != self)
		return;
	
	for (NSTabViewItem *item in [[_tabBarControl tabView] tabViewItems]) {
		if ([item identifier] == file) {
			[item setLabel:[file name]];
			break;
		}
	}
}
#pragma mark Callbacks
- (void)_activeBuildTargetHasNoInputFileAlertDidEnd:(NSAlert *)alert code:(NSInteger)code info:(void *)info {
	if (code != NSAlertDefaultReturn)
		return;
	
	[[alert window] close];
	
	[self editBuildTargets:nil];
}

- (void)_deleteAlertDidEnd:(NSAlert *)alert code:(NSInteger)code info:(void *)info {
	if (code == NSAlertAlternateReturn)
		return;
	
	if (code == NSAlertDefaultReturn)
		[self _deleteObjects:[[self currentViewController] selectedObjects] deleteFiles:NO];
	else if (code == NSAlertOtherReturn)
		[self _deleteObjects:[[self currentViewController] selectedObjects] deleteFiles:YES];
}
@end

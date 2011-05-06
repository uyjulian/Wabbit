//
//  RSMemoryTableView.h
//  WabbitStudio
//
//  Created by William Towe on 5/4/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import <AppKit/NSTableView.h>
#import "RSGotoAddressControllerProtocol.h"



@interface RSDebuggerMemoryTableView : NSTableView {
@private
    IBOutlet id <RSGotoAddressController> _gotoAddressController;
}

- (IBAction)gotoAddress:(id)sender;

@end

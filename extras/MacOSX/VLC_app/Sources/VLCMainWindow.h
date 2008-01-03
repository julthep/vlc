/*****************************************************************************
 * VLCMainWindow.h: VLCMainWindow implementation
 *****************************************************************************
 * Copyright (C) 2007 Pierre d'Herbemont
 * Copyright (C) 2007 the VideoLAN team
 * $Id$
 *
 * Authors: Pierre d'Herbemont <pdherbemont # videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#import <Cocoa/Cocoa.h>
#import "VLCController.h"
#import "VLCMediaArrayController.h"
#import "VLCAppAdditions.h"
#import "VLCBrowsableVideoView.h"

@interface VLCMainWindow : NSWindow {
    /* IB elements */
    IBOutlet id mediaListItemFetchedStatus;
    IBOutlet id mediaListItemsCount;
    IBOutlet id mediaListSearchField;

    IBOutlet NSOutlineView * categoriesListView;
    IBOutlet NSTableView * mediaListView;

    IBOutlet VLCBrowsableVideoView * videoView;
    IBOutlet id fillScreenButton;
    IBOutlet id fullScreenButton;
    IBOutlet NSSlider * mediaReadingProgressSlider;
    IBOutlet NSTextField * mediaReadingProgressText;
    IBOutlet NSTextField * mediaDescriptionText;

    IBOutlet id navigatorViewToggleButton;
    IBOutlet VLCOneSplitView * mainSplitView;
    IBOutlet NSView * navigatorView;
    IBOutlet NSView * videoPlayerAndControlView;
    IBOutlet NSView * controlView;

    IBOutlet NSButton * addPlaylistButton;
    IBOutlet NSButton * removePlaylistButton;

    /* Toolbar */
    IBOutlet NSView * toolbarMediaAudioVolume;
    IBOutlet NSView * toolbarMediaDescription;
    IBOutlet NSView * toolbarMediaControl;

    VLCMediaPlayer * mediaPlayer;

    IBOutlet VLCController * controller; /* This is a VLCController binded to the File's Owner of the nib */

    /* Controllers */
    NSTreeController * categoriesTreeController;
    VLCMediaArrayController * mediaArrayController;
    
    /* Window state */
    CGFloat navigatorHeight;
}

@property BOOL navigatorViewVisible;
@end

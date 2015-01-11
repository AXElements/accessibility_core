//
//  AppDelegate.swift
//  AXElementsTester
//
//  Created by Mark Rada on 2014-10-17.
//  Copyright (c) 2014 Marketcircle Incorporated. All rights reserved.
//

import Cocoa

@NSApplicationMain
class AppDelegate : NSObject, NSApplicationDelegate {

    @IBOutlet var window           : TesterWindow?
    @IBOutlet var yes_button       : NSButton?
    @IBOutlet var bye_button       : NSButton?
    @IBOutlet var scroll_area      : NSScrollView?
    @IBOutlet var menu             : NSMenu?
    @IBOutlet var array_controller : TesterArrayController?

    func applicationDidFinishLaunching(_ : AnyObject) {
        populate_table()
        set_identifiers()
        populate_menu()
    }

    func populate_table() {
        let attrs : NSArray = self.window!.accessibilityAttributeNames()
        attrs.enumerateObjectsUsingBlock { (name, _, _) in
            let value : NSObject? =
                self.window!.accessibilityAttributeValue(name as NSString) as? NSObject
            let row : TableRow =
                TableRow(init_name: name as NSString, init_value: value?.description)

            self.array_controller!.addObject(row)
        }
    }

    func set_identifiers() {
        self.yes_button!.identifier = "I'm a little teapot"
        self.scroll_area!.identifier = "Text Area"
    }

    func populate_menu() {
        for num in 1...50 {
            let item = NSMenuItem(title: String(num),
                                  action: nil,
                                  keyEquivalent: "")
            self.menu!.addItem(item)
        }
    }

    @IBAction func post_notification(_ : AnyObject) {
        NSAccessibilityPostNotification(yes_button!.cell(), "Cheezburger")
        (self.window!.contentView as NSView).addSubview(bye_button!)
    }

    @IBAction func remove_bye_button(_ : AnyObject) {
        self.bye_button!.removeFromSuperview()
    }

    @IBAction func orderFrontPreferencesPanel(_ : AnyObject) {
        let prefs = PrefPaneController(windowNibName: "PrefPane")
        prefs.loadWindow()
        prefs.showWindow(self)
    }
}

//
//  TesterWindow.swift
//  AXElementsTester
//
//  Created by Mark Rada on 2014-10-19.
//  Copyright (c) 2014 Marketcircle Incorporated. All rights reserved.
//

import Cocoa

let AXLol : NSString                  = "AXLol"
let AXPie : NSString                  = "AXPie"
let AXIsNyan : NSString               = "AXIsNyan"
let AXURLAttribute : NSString         = "AXURL"
let AXDescriptionAttribute : NSString = "AXDescription"
let AXData : NSString                 = "AXData"

class TesterWindow : NSWindow {

    let extra_attrs : [NSString] = [
        AXLol,
        AXPie,
        AXIsNyan,
        AXURLAttribute,
        AXDescriptionAttribute,
        AXData
    ]

    override func accessibilityAttributeNames() -> [Any] {
        return (super.accessibilityAttributeNames() as NSArray)
                .addingObjects(from: extra_attrs)
    }

    override func accessibilityAttributeValue(_ name : String) -> Any? {
        if (name == AXLol as String) {
            return NSValue(rect: CGRect.zero)
        }
        if (name == AXPie as String) {
            return NSValue(range: NSRange(location: 10,length: 10))
        }
        if (name == AXIsNyan as String) {
            return false
        }
        if (name == AXURLAttribute as String) {
            return URL(string: "http://macruby.org/")
        }
        if (name == AXDescriptionAttribute as String) {
            return "Test Fixture"
        }
        if (name == AXData as String) {
            return (try? Data(contentsOf: URL(fileURLWithPath: "/bin/cat")))
        }
        return super.accessibilityAttributeValue(name)
    }

}

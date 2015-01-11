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

    let extra_attrs : NSArray = [
        AXLol,
        AXPie,
        AXIsNyan,
        AXURLAttribute,
        AXDescriptionAttribute,
        AXData
    ]

    override func accessibilityAttributeNames() -> [AnyObject] {
        return (super.accessibilityAttributeNames() as NSArray)
                .arrayByAddingObjectsFromArray(extra_attrs)
    }

    override func accessibilityAttributeValue(name : String) -> AnyObject? {
        if (name == AXLol) {
            return NSValue(rect: CGRectZero)
        }
        if (name == AXPie) {
            return NSValue(range: NSRange(location: 10,length: 10))
        }
        if (name == AXIsNyan) {
            return false
        }
        if (name == AXURLAttribute) {
            return NSURL(string: "http://macruby.org/")
        }
        if (name == AXDescriptionAttribute) {
            return "Test Fixture"
        }
        if (name == AXData) {
            return NSData(contentsOfFile: "/bin/cat")
        }
        return super.accessibilityAttributeValue(name)
    }

}

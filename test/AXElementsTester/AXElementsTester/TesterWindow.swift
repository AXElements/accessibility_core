//
//  TesterWindow.swift
//  AXElementsTester
//
//  Created by Mark Rada on 2014-10-19.
//  Copyright (c) 2014 Marketcircle Incorporated. All rights reserved.
//

import Cocoa

let AXLol : String                  = "AXLol"
let AXPie : String                  = "AXPie"
let AXIsNyan : String               = "AXIsNyan"
let AXURLAttribute : String         = "AXURL"
let AXDescriptionAttribute : String = "AXDescription"

class TesterWindow : NSWindow {

    let extra_attrs : NSArray = [
        AXLol,
        AXPie,
        AXIsNyan,
        AXURLAttribute,
        AXDescriptionAttribute
    ]

    override func accessibilityAttributeNames() -> [AnyObject] {
        return (super.accessibilityAttributeNames() as NSArray)
                .arrayByAddingObjectsFromArray(extra_attrs)
    }

    override func accessibilityAttributeValue(name : String) -> AnyObject? {
        switch name {
        case AXLol    : return NSValue(rect: CGRectZero)
        case AXPie    : return NSValue(range: NSRange(location: 10,length: 10))
        case AXIsNyan : return false
        case AXURLAttribute : return NSURL(string: "http://macruby.org/")
        case AXDescriptionAttribute : return "Test Fixture"
        default : return super.accessibilityAttributeValue(name)
        }
    }

}

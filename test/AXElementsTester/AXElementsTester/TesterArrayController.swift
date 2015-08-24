//
//  TesterArrayController.swift
//  AXElementsTester
//
//  Created by Mark Rada on 2014-10-19.
//  Copyright (c) 2014 Marketcircle Incorporated. All rights reserved.
//

import Cocoa

class TesterArrayController : NSArrayController {

    override var selectsInsertedObjects: Bool {
        get {
            return true
        }
        set(derp) {
            // derp
        }
    }
}

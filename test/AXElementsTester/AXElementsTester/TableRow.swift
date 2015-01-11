//
//  TableRow.swift
//  AXElementsTester
//
//  Created by Mark Rada on 2014-10-20.
//  Copyright (c) 2014 Marketcircle Incorporated. All rights reserved.
//

import Foundation

@objc class TableRow : NSObject {

    var name  : NSString
    var value : NSString?

    init(init_name : NSString, init_value : NSString?) {
        name  = init_name
        value = init_value
    }
    
}
//
//  TableRow.swift
//  AXElementsTester
//
//  Created by Mark Rada on 2014-10-20.
//  Copyright (c) 2014 Marketcircle Incorporated. All rights reserved.
//

import Foundation

@objc class TableRow : NSObject {

    var name: String
    var val: String?

    func value() -> Any? {
        return val
    }

    init(init_name: String, init_value: String?) {
        name  = init_name
        val   = init_value
    }
    
}

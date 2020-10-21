//
//  Energy.swift
//  usingSmileLib
//
//  Created by Taewoo Kim on 16.07.18.
//  Copyright © 2018 Taewoo Kim. All rights reserved.
//

import Foundation

struct Energy : Codable {
    let msgType: String
    let msgName: String
    let sender: String
    let smileTime: Double
    let userTime1: Double
    let userTime2: Double
    let readerTime: Double
    let msgId: Int
    let floatData: FloatData
    let intData: IntData
    let msgText: String
    let userFlag1: Int
    let userFlag2: Int
    let userFlag3: Int
    let custData: Data?
    let custData2: Data?
    
    enum CodingKeys: String, CodingKey {
        case msgType = "msgtype"
        case msgName = "msgname"
        case sender = "sender"
        case smileTime = "smileTime"
        case userTime1 = "userTime1"
        case userTime2 = "userTime2"
        case readerTime = "readerTime"
        case msgId = "msgid"
        case floatData = "floatData"
        case intData = "intData"
        case msgText = "msgtext"
        case userFlag1 = "userflag1"
        case userFlag2 = "userflag2"
        case userFlag3 = "userflag3"
        case custData = "custData"
        case custData2 = "custData2"
    }
    
    init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: CodingKeys.self)
        msgType = try values.decode(String.self, forKey: .msgType)
        msgName = try values.decode(String.self, forKey: .msgName)
        sender = try values.decode(String.self, forKey: .sender)
        smileTime = try values.decode(Double.self, forKey: .smileTime)
        userTime1 = try values.decode(Double.self, forKey: .userTime1)
        userTime2 = try values.decode(Double.self, forKey: .userTime2)
        readerTime = try values.decode(Double.self, forKey: .readerTime)
        msgId = try values.decode(Int.self, forKey: .msgId)
        floatData = try values.decode(FloatData.self, forKey: .floatData)
        intData = try values.decode(IntData.self, forKey: .intData)
        msgText = try values.decode(String.self, forKey: .msgText)
        userFlag1 = try values.decode(Int.self, forKey: .userFlag1)
        userFlag2 = try values.decode(Int.self, forKey: .userFlag2)
        userFlag3 = try values.decode(Int.self, forKey: .userFlag3)
        custData = try? values.decode(Data.self, forKey: .custData)
        custData2 = try? values.decode(Data.self, forKey: .custData2)
    }
}

struct FloatData : Codable {
    var values: Array<Double>
    let _0: Double
    let _1: Double
    let _2: Double
    let _3: Double
    let _4: Double
    let _5: Double
    let _6: Double
    let _7: Double

    enum CodingKeys: String, CodingKey {
        case _0 = "0"
        case _1 = "1"
        case _2 = "2"
        case _3 = "3"
        case _4 = "4"
        case _5 = "5"
        case _6 = "6"
        case _7 = "7"
    }
    
    init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: CodingKeys.self)
        self.values = Array<Double>(repeating: 0.0, count: 8)
        self.values[0] = try values.decode(Double.self, forKey: ._0)
        self.values[1] = try values.decode(Double.self, forKey: ._1)
        self.values[2] = try values.decode(Double.self, forKey: ._2)
        self.values[3] = try values.decode(Double.self, forKey: ._3)
        self.values[4] = try values.decode(Double.self, forKey: ._4)
        self.values[5] = try values.decode(Double.self, forKey: ._5)
        self.values[6] = try values.decode(Double.self, forKey: ._6)
        self.values[7] = try values.decode(Double.self, forKey: ._7)
        _0 = self.values[0]
        _1 = self.values[1]
        _2 = self.values[2]
        _3 = self.values[3]
        _4 = self.values[4]
        _5 = self.values[5]
        _6 = self.values[6]
        _7 = self.values[7]
    }
}

struct IntData: Codable {
    var values: Array<Int>
    let _0: Int
    let _1: Int
    let _2: Int
    let _3: Int
    let _4: Int
    let _5: Int
    let _6: Int
    let _7: Int
 
    enum CodingKeys: String, CodingKey {
        case _0 = "0"
        case _1 = "1"
        case _2 = "2"
        case _3 = "3"
        case _4 = "4"
        case _5 = "5"
        case _6 = "6"
        case _7 = "7"
    }
    
    init(from decoder: Decoder) throws {
        let values = try decoder.container(keyedBy: CodingKeys.self)
        self.values = Array<Int>(repeating: 0, count: 8)
        self.values[0] = try values.decode(Int.self, forKey: ._0)
        self.values[1] = try values.decode(Int.self, forKey: ._1)
        self.values[2] = try values.decode(Int.self, forKey: ._2)
        self.values[3] = try values.decode(Int.self, forKey: ._3)
        self.values[4] = try values.decode(Int.self, forKey: ._4)
        self.values[5] = try values.decode(Int.self, forKey: ._5)
        self.values[6] = try values.decode(Int.self, forKey: ._6)
        self.values[7] = try values.decode(Int.self, forKey: ._7)
        _0 = self.values[0]
        _1 = self.values[1]
        _2 = self.values[2]
        _3 = self.values[3]
        _4 = self.values[4]
        _5 = self.values[5]
        _6 = self.values[6]
        _7 = self.values[7]
    }
}

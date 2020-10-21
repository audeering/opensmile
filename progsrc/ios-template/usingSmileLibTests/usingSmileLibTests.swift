//
//  usingSmileLibTests.swift
//  usingSmileLibTests
//
//  Created by Taewoo Kim on 11.05.18.
//  Copyright © 2018 Taewoo Kim. All rights reserved.
//

import XCTest
@testable import usingSmileLib

class usingSmileLibTests: XCTestCase {
    
    let json = """
        {
            "msgtype": "energy",
            "msgname": "energy_act",
            "sender": "bandEnergy00",
            "smileTime": 0.252718,
            "userTime1": 0.100000,
            "userTime2": 0.200000,
            "readerTime": 0.300000,
            "msgid": 12345,
            "floatData": {
                "0": 2.165087e+03,
                "1": 2.802422e+00,
                "2": 1.691887e+00,
                "3": 1.084233e+00,
                "4": 7.433313e-01,
                "5": 5.332878e-01,
                "6": 4.053593e-01,
                "7": 3.164835e-01
            },
            "intData": {
                "0": 10000,
                "1": 10001,
                "2": 10002,
                "3": 10003,
                "4": 10004,
                "5": 10005,
                "6": 10006,
                "7": 10007
            },
            "msgtext": "message text",
            "userflag1": 1,
            "userflag2": 2,
            "userflag3": 3,
            "custData": null,
            "custData2": null
        }
        """
    
    override func setUp() {
        super.setUp()
        // Put setup code here. This method is called before the invocation of each test method in the class.
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
        super.tearDown()
    }
    
    func test_energy_convertToStruct_exceptCustData() {
        let energy : Energy? = try? JSONDecoder().decode(Energy.self, from: json.data(using: .utf8)!)
        XCTAssertNotNil(energy)
        XCTAssertEqual(energy?.msgType, "energy")
        XCTAssertEqual(energy?.msgName, "energy_act")
        XCTAssertEqual(energy?.sender, "bandEnergy00")
        XCTAssertEqual(energy?.smileTime, 0.252718)
        XCTAssertEqual(energy?.userTime1, 0.1)
        XCTAssertEqual(energy?.userTime2, 0.2)
        XCTAssertEqual(energy?.readerTime, 0.3)
        XCTAssertEqual(energy?.msgId, 12345)
        XCTAssertEqual(energy?.floatData.values[0], 2.165087e+03)
        XCTAssertEqual(energy?.floatData.values[1], 2.802422e+00)
        XCTAssertEqual(energy?.floatData.values[2], 1.691887e+00)
        XCTAssertEqual(energy?.floatData.values[3], 1.084233e+00)
        XCTAssertEqual(energy?.floatData.values[4], 7.433313e-01)
        XCTAssertEqual(energy?.floatData.values[5], 5.332878e-01)
        XCTAssertEqual(energy?.floatData.values[6], 4.053593e-01)
        XCTAssertEqual(energy?.floatData.values[7], 3.164835e-01)
        XCTAssertEqual(energy?.floatData._0, energy?.floatData.values[0])
        XCTAssertEqual(energy?.floatData._1, energy?.floatData.values[1])
        XCTAssertEqual(energy?.floatData._2, energy?.floatData.values[2])
        XCTAssertEqual(energy?.floatData._3, energy?.floatData.values[3])
        XCTAssertEqual(energy?.floatData._4, energy?.floatData.values[4])
        XCTAssertEqual(energy?.floatData._5, energy?.floatData.values[5])
        XCTAssertEqual(energy?.floatData._6, energy?.floatData.values[6])
        XCTAssertEqual(energy?.floatData._7, energy?.floatData.values[7])
        XCTAssertEqual(energy?.intData.values[0], 10000)
        XCTAssertEqual(energy?.intData.values[1], 10001)
        XCTAssertEqual(energy?.intData.values[2], 10002)
        XCTAssertEqual(energy?.intData.values[3], 10003)
        XCTAssertEqual(energy?.intData.values[4], 10004)
        XCTAssertEqual(energy?.intData.values[5], 10005)
        XCTAssertEqual(energy?.intData.values[6], 10006)
        XCTAssertEqual(energy?.intData.values[7], 10007)
        XCTAssertEqual(energy?.intData._0, energy?.intData.values[0])
        XCTAssertEqual(energy?.intData._1, energy?.intData.values[1])
        XCTAssertEqual(energy?.intData._2, energy?.intData.values[2])
        XCTAssertEqual(energy?.intData._3, energy?.intData.values[3])
        XCTAssertEqual(energy?.intData._4, energy?.intData.values[4])
        XCTAssertEqual(energy?.intData._5, energy?.intData.values[5])
        XCTAssertEqual(energy?.intData._6, energy?.intData.values[6])
        XCTAssertEqual(energy?.intData._7, energy?.intData.values[7])
        XCTAssertEqual(energy?.msgText, "message text")
        XCTAssertEqual(energy?.userFlag1, 1)
        XCTAssertEqual(energy?.userFlag2, 2)
        XCTAssertEqual(energy?.userFlag3, 3)
        XCTAssertEqual(energy?.custData, nil)
        XCTAssertEqual(energy?.custData2, nil)
    }
}


























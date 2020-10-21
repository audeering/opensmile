//
//  usingSmileLibUITests.swift
//  usingSmileLibUITests
//
//  Created by Taewoo Kim on 11.05.18.
//  Copyright © 2018 Taewoo Kim. All rights reserved.
//

import XCTest

class usingSmileLibUITests: XCTestCase {
        
    override func setUp() {
        super.setUp()
        
        // Put setup code here. This method is called before the invocation of each test method in the class.
        
        // In UI tests it is usually best to stop immediately when a failure occurs.
        continueAfterFailure = false
        // UI tests must launch the application that they test. Doing this in setup will make sure it happens for each test method.
        XCUIApplication().launch()

        // In UI tests it’s important to set the initial state - such as interface orientation - required for your tests before they run. The setUp method is a good place to do this.
    }
    
    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
        super.tearDown()
    }
    
    func testExample() {
        
        let app = XCUIApplication()
        let recordingButton = app.buttons["Recording"]
        let externalButton = app.buttons["External"]
        let stopButton = app.buttons["Stop"]

        for _ in 0...3 {
            recordingButton.tap()
            XCTAssertFalse(recordingButton.isEnabled)
            XCTAssertFalse(externalButton.isEnabled)
            XCTAssertTrue(stopButton.isEnabled)
            sleep(3)
            stopButton.tap()
            XCTAssertTrue(recordingButton.isEnabled)
            XCTAssertTrue(externalButton.isEnabled)
            XCTAssertFalse(stopButton.isEnabled)
            sleep(1)
            
            externalButton.tap()
            XCTAssertFalse(recordingButton.isEnabled)
            XCTAssertFalse(externalButton.isEnabled)
            XCTAssertTrue(stopButton.isEnabled)
            sleep(3)
            stopButton.tap()
            XCTAssertTrue(recordingButton.isEnabled)
            XCTAssertTrue(externalButton.isEnabled)
            XCTAssertFalse(stopButton.isEnabled)
            sleep(1)
        }

        // Use recording to get started writing UI tests.
        // Use XCTAssert and related functions to verify your tests produce the correct results.
    }
    
}

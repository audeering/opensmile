import UIKit

/**
 Main view controller using openSMILE library.
 
 - Copyright: Copyright (c) audEERING GmbH. All rights reserved.
 - Date: 22.06.2018
 - Author: Taewoo Kim(tkim@audeering.com)
 */
class ViewController: UIViewController {
    @IBOutlet weak var startButton: UIButton!
    @IBOutlet weak var externalButton: UIButton!
    @IBOutlet weak var stopButton: UIButton!
    @IBOutlet weak var resetModeButton: UIButton!
    @IBOutlet weak var messageLabel: UILabel!
    @IBOutlet weak var timerLabel: UILabel!
    @IBOutlet weak var barView: BarView!
    
    @objc dynamic var openSmile: OpenSmile?
    var startTime: Date?
    var observation :NSKeyValueObservation?
    var sourceName: String?
    var readyToWrite: Bool = false
    var isResetMode: Bool = false
}


// MARK: - Override function
extension ViewController {
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.observation = self.observe(\ViewController.openSmile, options: [.initial]) { (vc, observedChange) in 
            if let _ = vc.openSmile {
                DispatchQueue.main.async {
                    vc.startButton.isEnabled = false;
                    vc.externalButton.isEnabled = false;
                    vc.stopButton.isEnabled = true;
                }
            }
            else {
                DispatchQueue.main.async {
                    vc.startButton.isEnabled = true;        vc.startButton.isSelected = false;
                    vc.externalButton.isEnabled = true;     vc.externalButton.isSelected = false;
                    vc.stopButton.isEnabled = false;        vc.stopButton.isSelected = false;
                }
            }
        }
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

// MARK: - Button event
extension ViewController {
        
    @IBAction func onStartButton(_ sender: UIButton) {
        sourceName = "liveIn"   // Use an internal recorder of opensmile as audio input
        openSmile = OpenSmile()

        DispatchQueue.global(qos: .default).async {
            let confFile = "liveinput".fullPathInBundle()
            let outwav  = "output.wav".fullPathInDocuments()
            
            do {
                try self.openSmile?.initialize(confFile, argument: ["-l", "2", "-O", outwav])
                repeat {
                    self.configSmileIosCallback()
                    try self.openSmile?.run()
                    self.openSmile?.reset()
                } while self.isResetMode
            } catch let error as NSError {
                print(error)
            }
            
            self.openSmile = nil
            self.startTime = nil
        }
        
        // Update UI
        startTime = Date()
        DispatchQueue.global(qos: .default).async {
            while let start = self.startTime {
                let diff = Date().timeIntervalSince(start)
                DispatchQueue.main.async {
                    self.timerLabel.text = diff.stringFormatted()
                }
                usleep(1000000 / 10); // 0.1 seconds
            }
        }
    }
    
    @IBAction func onExternalButton(_ sender: UIButton) {
        sourceName = "externalIn"       // Receive audio input from an external source
        openSmile = OpenSmile()
        readyToWrite = false
        
        // Run opensmile
        DispatchQueue.global(qos: .default).async {
            let confFile = "externalinput".fullPathInBundle()
            let outwav  = "output.wav".fullPathInDocuments()
            
            do {
                try self.openSmile?.initialize(confFile, argument: ["-l", "2", "-O", outwav])
                repeat {
                    self.configSmileIosCallback()
                    self.readyToWrite = true
                    try self.openSmile?.run()
                    self.readyToWrite = false
                    self.openSmile?.reset()
                } while self.isResetMode
            } catch let error as NSError {
                print(error)
            }

            self.openSmile = nil
            self.startTime = nil
        }
        
        // Update UI and write random data to opensmile
        startTime = Date()
        DispatchQueue.global(qos: .default).async {
            while let start = self.startTime {
                if false == self.readyToWrite {
                    usleep(1000000 / 10)
                    continue
                }
                
                // Update UI
                let diff = Date().timeIntervalSince(start)
                DispatchQueue.main.async {
                    self.timerLabel.text = diff.stringFormatted()
                }
                
                // Send random data to opensmile.
                let len = 128
                let buf = UnsafeMutablePointer<Float32>.allocate(capacity: len)
                buf.initialize(repeating: 0xff, count: len)
                if let smileIos = self.openSmile {
                    smileIos.writeData(toExternalAudioSource: self.sourceName, data: buf, length: UInt32(len))
                }
                else {
                    buf.deallocate()
                    return;
                }
                
                buf.deallocate()
                usleep(1000000 / 100); // 0.01 sec
            }
        }
    }

    @IBAction func onResetModeButton(_ sender: UIButton) {
        resetModeButton.isSelected = !resetModeButton.isSelected
        isResetMode = resetModeButton.isSelected
    }
    
    @IBAction func onStopButton(_ sender: UIButton) {
        openSmile?.abort()
    }
}


// MARK: - SmileIos callback
extension ViewController {
    func configSmileIosCallback() {
        guard let smileIos = self.openSmile else {
            return
        }
        
        //
        // Set data message callback
        //
        let dataClosure00 =  {(data: UnsafeMutablePointer<Float32>?, size: UInt32) -> Void in
            print("data closure00, received data size = \(size)")
            let bufferPointer = UnsafeBufferPointer(start: data, count: Int(size))
            DispatchQueue.main.async {
                self.smileIosExternalSinkCallback(bufferPointer);
            }
        }
        let dataClosure01 =  {(data: UnsafeMutablePointer<Float32>?, size: UInt32) -> Void in
            print("data closure01, received data size = \(size)")
        }
        let dataClosure02 =  {(data: UnsafeMutablePointer<Float32>?, size: UInt32) -> Void in
            print("data closure02, received data size = \(size)")
        }
        smileIos.setExternalSinkCallback("externalSink00", callback: dataClosure00)
        smileIos.setExternalSinkCallback("externalSink01", callback: dataClosure01)
        smileIos.setExternalSinkCallback("externalSink02", callback: dataClosure02)
        
        //
        // Set json message callback
        //
        let msgClosure00 =  {(json: String?) -> Void in
            print("message closure00, received message")
            DispatchQueue.main.async {
                self.smileIosExternalMessageInterfaceCallback(json);
            }
        }
        let msgClosure01 =  {(json: String?) -> Void in
            print("message closure01, received message")
        }
        let msgClosure02 =  {(json: String?) -> Void in
            print("message closure02, received message")
        }
        smileIos.setExternalJsonMessageInterfaceCallback("externalMessageInterface00", callback: msgClosure00)
        smileIos.setExternalJsonMessageInterfaceCallback("externalMessageInterface01", callback: msgClosure01)
        smileIos.setExternalJsonMessageInterfaceCallback("externalMessageInterface02", callback: msgClosure02)
    }
    
    func smileIosExternalMessageInterfaceCallback (_ json: String? ) {
        guard let json = json else {
            return
        }
        
        messageLabel?.text = json;
        
        if let data = json.data(using: .utf8),
            let energy = try? JSONDecoder().decode(Energy.self, from: data) {
            let ary = energy.floatData.values.map({Float32($0)})
            barView?.values = ary.map({
                if -200 <= $0 && $0 <= 0.0 {
                    return 1.0 - ($0 / -200.0)
                }
                else {
                    return $0 / 10.0
                }
            })
        }
    }

    func smileIosExternalSinkCallback (_ bufferPointer: UnsafeBufferPointer<Float32>) {
        let strAry = bufferPointer.enumerated().map({ (index, element) in
            return String("\(index): \(element)")
        })
        
        messageLabel?.text = strAry.joined(separator: "\n")
        barView?.values = bufferPointer.map({
            if -200 <= $0 && $0 <= 0.0 {
                return 1.0 - ($0 / -200.0)
            }
            else {
                return $0 / 10.0
            }
        })
    }
}

// MARK: - ViewController
extension ViewController {
    func saveTextToFile (_ text: String?, _ fileName: String) {
        guard let text = text else {
            return
        }
        
        do {
            if let documentDirectory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
                let fileURL = documentDirectory.appendingPathComponent(fileName)
                try text.write(to: fileURL, atomically: false, encoding: .utf8)
            }
        } catch {
            print("error in saveing text:", error)
        }
    }
}

// MARK: - TimeInterval
extension TimeInterval {
    // builds string in format 00:00.0
    func stringFormatted() -> String {
        var miliseconds = self * 10
        miliseconds = miliseconds.truncatingRemainder(dividingBy: 10)
        let interval = Int(self)
        let seconds = interval % 60
        let minutes = (interval / 60) % 60
        return String(format: "%02d:%02d.%.f", minutes, seconds, miliseconds)
    }
}

// MARK: - String
extension String {
    func fullPathInBundle() -> String {
        if let path = Bundle.main.path(forResource: self, ofType: "conf", inDirectory: "iosConfig") {
            return path
        }
        else {
            return ""
        }
    }

    func fullPathInDocuments() -> String {
        let documentsUrl = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        let path = documentsUrl.appendingPathComponent(self)
        return path.path
    }
}
















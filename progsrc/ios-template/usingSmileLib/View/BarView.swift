import UIKit

/**
 Display vertical bar.

 - Note: Set value between 0.0 ~ 1.0. 
 - Copyright: Copyright (c) audEERING GmbH. All rights reserved.
 - Date: 22.06.2018
 - Author: Taewoo Kim(tkim@audeering.com)
 */
class BarView: UIView {
    var values : Array<Float32>? {
        didSet {
            setNeedsDisplay()
        }
    }
    
    override func awakeFromNib() {
        super.awakeFromNib()
    }
    
    override func draw(_ rect: CGRect) {
        guard let values = values else {
            return
        }
        
        UIColor.red.set()
        let aPath = UIBezierPath()
        let totCnt = values.count
        let barW = bounds.size.width / CGFloat(totCnt)
        let margin = bounds.size.width / CGFloat(totCnt * 3)
        aPath.lineWidth = barW - margin
        var barX = CGFloat(barW / 2.0)
        
        for (_, value) in values.enumerated() {
            aPath.move(to: CGPoint(x: barX, y: bounds.size.height))
            let barH = bounds.size.height * CGFloat(value)
            let barY = bounds.size.height - barH
            aPath.addLine(to: CGPoint(x: barX, y: barY))
            barX = barX + barW
        }
        
        aPath.close()
        aPath.stroke()
    }
}


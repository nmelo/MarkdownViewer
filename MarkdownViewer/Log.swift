//
//  Log.swift
//  MarkdownViewer
//
//  Created by Sbarex on 21/03/22.
//

import Foundation
import OSLog

extension OSLog {
    private static let subsystem = "org.anarion.MarkdownViewer"

    static let quickLookExtension = OSLog(subsystem: subsystem, category: "Renderer")
    static let rendering = OSLog(subsystem: subsystem, category: "Rendering")
}

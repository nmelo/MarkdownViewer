//
//  main.swift
//  MarkdownViewer
//
//  Explicit entry point. We don't use @main because, with no NSMainStoryboardFile
//  or NSMainNibFile in Info.plist, the Swift-generated `NSApplicationMain` never
//  instantiates our AppDelegate. Doing it ourselves wires the delegate up before
//  the run loop starts so applicationWillFinishLaunching / applicationDidFinishLaunching
//  actually fire.
//

import Cocoa

let app = NSApplication.shared
app.setActivationPolicy(.regular)

let delegate = AppDelegate()
app.delegate = delegate

app.run()

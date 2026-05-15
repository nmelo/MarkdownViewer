# MarkdownViewer

A standalone macOS markdown viewer with live file reload. Opens a `.md` file, renders it, and re-renders automatically on every save.

Fork of [sbarex/QLMarkdown](https://github.com/sbarex/QLMarkdown), trimmed down from a Quick Look extension into a plain app.

## Build

Requires Xcode 26 and `cmake` (for the bundled cmark-gfm build).

```sh
xcodebuild -project MarkdownViewer.xcodeproj \
  -scheme MarkdownViewer \
  -configuration Release \
  CODE_SIGN_IDENTITY="-" CODE_SIGN_STYLE=Manual \
  ONLY_ACTIVE_ARCH=YES build
```

The product lands in `~/Library/Developer/Xcode/DerivedData/MarkdownViewer-*/Build/Products/Release/MarkdownViewer.app`. Drop it in `/Applications`, then `codesign --force --sign - --deep` it for local launch.

## Open a file

- Finder: double-click or right-click → Open With
- Terminal: `open -a MarkdownViewer file.md`
- Drag-and-drop onto the window

## License

Inherits the original project's MIT license. See `LICENSE.txt`.

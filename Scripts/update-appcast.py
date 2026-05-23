#!/usr/bin/env python3
"""Insert a new release item at the top of appcast.xml.

Usage: update-appcast.py <shortVersion> <buildNumber> <edSignature> <length>

Idempotent: if an <item> with this shortVersionString already exists, the
script exits cleanly without modifying the file.
"""
import sys
import re
import email.utils
from pathlib import Path

APPCAST = Path(__file__).resolve().parent.parent / "appcast.xml"


def main() -> int:
    if len(sys.argv) != 5:
        print(__doc__, file=sys.stderr)
        return 2
    short, build, sig, length = sys.argv[1:5]
    pubdate = email.utils.formatdate(localtime=True)

    src = APPCAST.read_text()

    # Don't double-insert if this version is already in the feed.
    if f"<sparkle:shortVersionString>{short}</sparkle:shortVersionString>" in src:
        print(f"appcast already has {short} — leaving file alone")
        return 0

    item = f"""        <item>
            <title>Version {short}</title>
            <link>https://github.com/nmelo/MarkdownViewer/releases/tag/v{short}</link>
            <sparkle:version>{build}</sparkle:version>
            <sparkle:shortVersionString>{short}</sparkle:shortVersionString>
            <sparkle:minimumSystemVersion>11.0</sparkle:minimumSystemVersion>
            <pubDate>{pubdate}</pubDate>
            <description><![CDATA[
                <p>See <a href="https://github.com/nmelo/MarkdownViewer/releases/tag/v{short}">release notes</a>.</p>
            ]]></description>
            <enclosure
                url="https://github.com/nmelo/MarkdownViewer/releases/download/v{short}/MarkdownViewer-v{short}.zip"
                sparkle:version="{build}"
                sparkle:shortVersionString="{short}"
                sparkle:edSignature="{sig}"
                length="{length}"
                type="application/octet-stream" />
        </item>
"""

    # Newest items come first; insert immediately after <language>en</language>.
    new_src, count = re.subn(
        r"(<language>en</language>\s*\n)",
        r"\1" + item,
        src,
        count=1,
    )
    if count == 0:
        print("ERROR: couldn't find <language>en</language> anchor", file=sys.stderr)
        return 1

    APPCAST.write_text(new_src)
    print(f"Inserted item for v{short}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

gethtmlX is a simple html querier that get information from html.

Usage: gethtmlx <operations> [html-file]
Encoding caution: UTF8 desired! Or strange things could happen.
operation examples:
    getElementById(main).getElementsByClassName(list)[0].getAttribute(href)
    getElementById(main).getElementsByClassName(list)[0].textContent
    getElementById(main).getElementsByClassName("a b").length
    getElementById(main).getElementsByTagName(a)
    getElementById(main).children.length
    getElementsByTagName(a).each(getAttribute(href))
    document
    [document.]children
examples:
    type test.htm | gethtmlx getElementsByTagName(a)
    gethtmlx getElementsByTagName(a) < test.htm
    type ss.htm |gethtmlx getElementsByClassName(col-sm-4) |gethtmlx getElementsByTagName(h4).each(textContent)
Tips: You may use this together with iconv.

https://github.com/lifenjoiner/gethtmlX

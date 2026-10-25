-- Lets LaTeX break inline code, such as a file path or a qualified name, after a slash, a dot, a colon or an
-- underscore, without inserting a hyphen, so that a long name does not run into the margin.
local escapes = {
  ["\\"] = "\\textbackslash{}", ["{"] = "\\{", ["}"] = "\\}", ["$"] = "\\$", ["&"] = "\\&", ["#"] = "\\#",
  ["^"] = "\\textasciicircum{}", ["_"] = "\\_", ["%"] = "\\%", ["~"] = "\\textasciitilde{}",
}
local breaks = { ["/"] = true, ["."] = true, [":"] = true, ["_"] = true }

function Code(element)
  if not FORMAT:match("latex") then
    return nil
  end
  local text = {}
  for character in element.text:gmatch(utf8.charpattern) do
    table.insert(text, escapes[character] or character)
    if breaks[character] then
      table.insert(text, "\\allowbreak{}")
    end
  end
  return pandoc.RawInline("latex", "\\texttt{" .. table.concat(text) .. "}")
end

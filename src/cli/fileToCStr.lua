local sep = false
print(
'static const char '
.. arg[2]
.. '[] = "'
.. io.open(arg[1]):read('*a')
:gsub('.', function(c) 
local b = string.byte(c)
if sep and c:find('[a-fA-F]') then c = '" "' ..c end
sep=false
if b==10 or b==13 then return '\\n'
elseif b==92 then return '\\\\'
elseif b<32 or b>=127 then sep=true; return string.format('\\x%02X', b)
else return c end
end)
.. '";')

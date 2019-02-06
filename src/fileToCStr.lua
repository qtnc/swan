local sep = false
local varname = arg[1]

local function readall (l)
local r, t = {}, {}
for i = 2, #l do table.insert(t, l[i]) end
table.sort(t)
for _,f in pairs(t) do 
table.insert(r, io.open(f):read('*a'))
end
return table.concat(r, '\n')
end

print(
'static const char '
.. varname
.. '[] = "'
.. readall(arg)
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

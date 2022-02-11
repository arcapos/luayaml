local yaml = require 'yaml'

print(yaml._DESCRIPTION)
print(yaml._VERSION)
print(yaml._COPYRIGHT)
print()

local function dump(t, indent)
	for k, v in pairs(t) do
		for n = 1, indent or 0 do
			io.write('\t')
		end

		if type(k) == 'string' then
			io.write(k)
		end
		if type(v) == 'table' then
			io.write(':\n')
			dump(v, (indent or 0) + 1)
		elseif type(v) == 'boolean' then
			io.write(v and ' !!bool true\n' or  ' !!bool false\n')
		elseif type(v) == 'function' then
			io.write('\nLua function:\n')
			v()
		else
			io.write(' = ' .. v .. '\n')
		end
	end
	if indent == nil then
		print()
	end
end

print('Parse YAML data from a string:')
local data = yaml.parse([[
animals:
  - Horse
  - Dog
  - Cat
]])
dump(data)


print('Parse YAML data from a file:')
data = yaml.parsefile('test.yaml')
dump(data)

data = yaml.parsefile('numbers.yaml')
dump(data)

if yaml.verbosity ~= nil then
	yaml.verbosity(1)
end
data = yaml.parsefile('report2.yaml')
dump(data)

print('Parse a function from yaml file:')
data = yaml.parsefile('luafunction.yaml')
data.myFunction()
print(data.myValue)
print(data.myFile)

print('Parse YAMl data from a non-existent file:')

-- This will throw an error
local status, result = pcall(yaml.parsefile, 'nonexistent.yaml')
if status == false then
	print('yaml.parsefile failed: ' .. result)
end

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

		io.write(k)
		if type(v) == 'table' then
			io.write(':\n')
			dump(v, (indent or 0) + 1)
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

print('Parse YAMl data from a non-existent file:')

-- This will throw an error
local status, result = pcall(yaml.parsefile, 'nonexistant.yaml')
if status == false then
	print('yaml.parsefile failed: ' .. result)
end

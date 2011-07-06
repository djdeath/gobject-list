#!/usr/bin/env ruby

reg = /^ - (\w[\w\d]+): (\d+)/

class MexClass
        attr_reader :name, :nb

        def initialize name,nb
                @name, @nb = name, nb
        end

        def <=>(value)
                return @nb <=> value.nb if @nb != value.nb
                @name <=> value.name
        end

        def to_s
                "#{@name}: #{@nb}"
        end
end

classes = []

File.open(ARGV[0], "r").each_line do |line|
        if reg.match(line)
                classes << MexClass.new($1,$2.to_i)
        end
end

classes.sort!.reverse!

classes.each do |c|
        puts c if c.nb > 0
end

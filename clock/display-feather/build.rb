# rewrites input files using configuration variables from conf.yml
require 'erb'
require 'yaml'

class BuildConf
	attr_accessor :config

	def initialize
		@rootdir  = File.expand_path(File.join(File.dirname(__FILE__)))
		@srcdir   = File.join(@rootdir, 'srcfiles')
		@builddir = File.join(@rootdir, 'src')
		@config   = YAML.load_file(File.join(@rootdir, 'conf.yml'))
	end

	def build(file)
		b = binding
		newfile = ERB.new(File.read(File.join(@srcdir, file))).result(b)
		if newfile && newfile.size > 0
			target = File.basename(file)
			File.open(File.join(@builddir, file), "wb") {|f| f << newfile }
		end
	end
end

builder = BuildConf.new
ARGV.each {|file|
	builder.build(file)
}

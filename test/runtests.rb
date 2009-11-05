#!/usr/bin/env ruby

# if we're talkin' ruby 1.9, we'll use built in json, otherwise use
# the pure ruby library sittin' here
$:.push(File.dirname(__FILE__))
begin
  require 'json'
rescue LoadError
  require "json/json.rb"
end

sr = File.join(File.dirname(__FILE__), "..", "..", "bpsdk", "bin",
               "ServiceRunner")

clet = File.join(File.dirname(__FILE__), "..", "src", "build", "ImageAlter")

# XXX: defer to env var ?
raise "can't execute ServiceRunner: #{sr}" if !File.executable? sr
raise "can't find built service to test: #{clet}" if !File.directory? clet

def mypread(pio, timeo)
  output = String.new
  puts "selecting on #{pio}"
  while nil != select( [ pio ], nil, nil, timeo )  
    puts "selecting got data!"
    output += pio.sysread(1024) 
  end
  puts "returning #{output.length} bytes (#{output})"
  output
end

IO.popen("#{sr} -log debug #{clet}", "w+") do |srp|
  puts "spawned ServiceRunner"
  # discard startup stuff
  mypread(srp, 0.5)
  puts "allocating instance"
  srp.syswrite "allocate\n"
  mypread(srp, 0.5)

  # now let's iterate through all of our tests
  Dir.glob(File.join(File.dirname(__FILE__), "cases", "*.json")).each do |f|
    puts "-- #{f} -- "
    json = JSON.parse(File.read(f))
    # now let's change the 'file' param to a absolute URI
    p = File.join(File.dirname(__FILE__), "test_images", json["file"])
    p = File.expand_path(p)
    # now convert p into a file url
    json["file"] = "file://" + p
    cmd = JSON.generate(json).gsub("'", "\\'")
    cmd = "inv transform '#{cmd}'\n"
    x = srp.syswrite cmd
    puts "wrote #{x} of #{cmd.length} [#{cmd}]"
    rez = mypread(srp, 5.0)
    puts rez
  end
  puts "done!"
end

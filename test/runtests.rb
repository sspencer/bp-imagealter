#!/usr/bin/env ruby

require 'uri'

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

def mypread(pio, timeo, returnAfterInput = false)
  output = String.new
  while nil != select( [ pio ], nil, nil, timeo )  
    output += pio.sysread(1024) 
    break if output.length && returnAfterInput 
  end
  output
end

rv = 0

IO.popen("#{sr} #{clet}", "w+") do |srp|
  puts "spawned ServiceRunner"
  # discard startup stuff
  mypread(srp, 0.5, true)
  puts "allocating instance"
  srp.syswrite "allocate\n"
  mypread(srp, 0.5, true)

  tests = 0
  successes = 0

  # now let's iterate through all of our tests
  Dir.glob(File.join(File.dirname(__FILE__), "cases", "*.json")).each do |f|
    tests += 1 
    $stdout.write "#{File.basename(f, ".json")}: "
    json = JSON.parse(File.read(f))
    # now let's change the 'file' param to a absolute URI
    p = File.join(File.dirname(__FILE__), "test_images", json["file"])
    p = File.expand_path(p)
    # now convert p into a file url
    json["file"] = "file://" + p
    cmd = JSON.generate(json).gsub("'", "\\'")
    # NOTE: the appended "show" command.  Service runner as of
    #       2.4.20 has a bug where it doesn't flush output.  So the
    #       results of invocation can be stuck in service runner's
    #       output buffer.  The 'show' command causes a flush.
    #       fix committed for 2.5.x branch, and we can remove this
    #       once that's live 
    srp.syswrite "inv transform '#{cmd}'\nshow\n"
    rez = mypread(srp, 1.0, true)

    # now rez is of the form >{ "file": "file:///foo.x" } 1 instance...<
    # we'll strip off the trailing gunk, extract the file url
    # and compare it to the .out file.  if we faqil anywhere along this
    # path, then we'll give up and call it a failure
    imgGot = nil
    begin
      robj = JSON.parse(rez.sub(/\}.*$/m, '}'))
      gotImgPath = URI.parse(robj['file']).path
      imgGot = File.open(gotImgPath, "rb") { |oi| oi.read }
      wantImgPath = File.join(File.dirname(f),
                              File.basename(f, ".json") + ".out")
      raise "no output file for test!" if !File.exist? wantImgPath
      imgWant = File.open(wantImgPath, "rb") { |oi| oi.read }
      raise "output mismatch" if imgGot != imgWant
      # yay!  it worked!
      successes += 1
      puts "ok."
    rescue => e
      err = e.to_s
      # for convenience, if the test fails, we'll *save* the output
      # image in xxx.got
      if imgGot != nil
        gotPath = File.join(File.dirname(f),
                            File.basename(f, ".json") + ".got")
        File.open(gotPath, "wb") { |oi| oi.write(imgGot) }
        err += " [left result in #{File.basename(gotPath)}]"
      end
      puts "fail (#{err})"
    end
  end
  puts "#{successes}/#{tests} tests completed successfully"
  
  rv = successes == tests
end

exit rv

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

# attempt to find the ServiceRunner binary, a part of the BrowserPlus
# SDK. (http://browserplus.yahoo.com)
def findServiceRunner
  # first, try relative to this repo 
  srBase = File.join(File.dirname(__FILE__), "..", "..", "bpsdk", "bin")
  candidates = [
   File.join(srBase, "ServiceRunner.exe"),
   File.join(srBase, "ServiceRunner"),             
  ]
  
  # now use BPSDK_PATH env var if present
  if ENV.has_key? 'BPSDK_PATH'
    candidates.push File.join(ENV['BPSDK_PATH'], "bin", "ServiceRunner.exe")
    candidates.push File.join(ENV['BPSDK_PATH'], "bin", "ServiceRunner")
  end

  if ENV.has_key? 'SERVICERUNNER_PATH'
    candidates.push(ENV['SERVICERUNNER_PATH'])
  end

  candidates.each { |p|
    return p if File.executable? p
  }
  nil
end

sr = findServiceRunner

clet = File.join(File.dirname(__FILE__), "..", "src", "build", "ImageAlter")

# XXX: defer to env var ?
raise "can't execute ServiceRunner: #{sr}" if !File.executable? sr
raise "can't find built service to test: #{clet}" if !File.directory? clet

# arguments are a string that must match the test name
substrpat = ARGV.length ? ARGV[0] : ""

# perform a blocking read.  the third parameter is a magic duck:
# 1. if it evaluates to false, we'll block the full timeo
# 2. if it is a pattern, we'll  block until either timeo expires OR
#    we get output which matches the pattern
def mypread(pio, timeo, lookFor = false)
  output = String.new
  while nil != select( [ pio ], nil, nil, timeo )  
    output += pio.sysread(1024) 
    break if output.length && lookFor && output =~ lookFor
  end
  output
end

rv = 0

IO.popen("#{sr} #{clet}", "w+") do |srp|
  puts "Running ImageAlter tests "
  puts "(containing '#{substrpat}')" if substrpat && substrpat.length > 0
  # discard startup output
  mypread(srp, 0.5, /service initialized/)
  srp.syswrite "allocate\n"
  mypread(srp, 0.5, /allocated/)

  tests = 0
  successes = 0

  # now let's iterate through all of our tests
  Dir.glob(File.join(File.dirname(__FILE__), "cases", "*.json")).each do |f|
    next if substrpat && substrpat.length > 0 && !f.include?(substrpat)
    tests += 1 
    $stdout.write "#{File.basename(f, ".json")}: "
    $stdout.flush
    json = JSON.parse(File.read(f))
    # now let's change the 'file' param to a absolute URI
    p = File.join(File.dirname(__FILE__), "test_images", json["file"])
    p = File.expand_path(p)
    # now convert p into a file url
    json["file"] = ((p[0] == "/") ? "file://" : "file:///" ) + p
    cmd = JSON.generate(json).gsub("'", "\\'")
    # NOTE: the appended "show" command.  Service runner as of
    #       2.4.20 has a bug where it doesn't flush output.  So the
    #       results of invocation can be stuck in service runner's
    #       output buffer.  The 'show' command causes a flush.
    #       fix committed for 2.5.x branch, and we can remove this
    #       once that's live 
    
    took = Time.now
    srp.syswrite "inv transform '#{cmd}'\nshow\n"
    rez = mypread(srp, 5.0, /allocated:/)
    took = Time.now - took

    # now rez is of the form >{ "file": "file:///foo.x" } 1 instance...<
    # we'll strip off the trailing gunk, extract the file url
    # and compare it to the .out file.  if we faqil anywhere along this
    # path, then we'll give up and call it a failure
    imgGot = nil
    begin
      robj = JSON.parse(rez.sub(/^.*\{/m, '{').sub(/\}.*$/m, '}'))
      gotImgPath = URI.parse(robj['file']).path
      gotImgPath.sub!(/^\//, "") if gotImgPath =~ /^\/[a-zA-Z]:/ 
      imgGot = File.open(gotImgPath, "rb") { |oi| oi.read }
      wantImgPath = File.join(File.dirname(f),
                              File.basename(f, ".json") + ".out")
      raise "no output file for test!" if !File.exist? wantImgPath
      imgWant = File.open(wantImgPath, "rb") { |oi| oi.read }
      raise "output mismatch" if imgGot != imgWant
      # yay!  it worked!
      successes += 1
      puts "ok. (#{robj['orig_width']}x#{robj['orig_height']} -> #{robj['width']}x#{robj['height']} took #{took}s)"
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
      puts "fail (#{err} took #{took}s)"
    end
  end
  puts "#{successes}/#{tests} tests completed successfully"
  
  rv = successes == tests
end

exit rv

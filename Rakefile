# Wrapper of CMAKE
[
 ENV["EXTRA_CMAKE_MODULES_DIRS"],
 File.join(File.dirname(__FILE__), "../cmake"), # in same top directory
 File.join(File.dirname(__FILE__), "../../cmake--master/workspace") # for hudson
].each do |dir|
  next unless dir
  dir = File.expand_path(dir)
  if File.exists? File.join(dir, "Findizenelib.cmake")
    $: << File.join(dir, "lib")
    ENV["EXTRA_CMAKE_MODULES_DIRS"] = dir
  end
end

require "izenesoft/project-finder"
# Must find default dependent projects before require izenesoft/tasks,
# which will load env.yml or cmake.yml and override the values found here.
finder = IZENESOFT::ProjectFinder.new(File.dirname(__FILE__))
finder.find_izenelib
finder.find_ilplib
finder.find_imllib
finder.find_libxml2
finder.find_kma
finder.find_icma
finder.find_ijma

require "izenesoft/tasks"

task :default => :cmake


task :resource, :user, :version do |t, args|
    mkdir "resource" unless File.exist? "resource"
    version = args[:version] || "hawk"
    user_host = "izenesoft.cn"
    if args[:user]
      user_host = "#{args[:user]}@izenesoft.cn"
    end
    sh "rsync -avP --del #{user_host}:/data/sf1r-resource/#{version}/resource/ resource/"
  end

IZENESOFT::CMake.new do |t|
  t.source_dir = "."
end

IZENESOFT::BoostTest.new

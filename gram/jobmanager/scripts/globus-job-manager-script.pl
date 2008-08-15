#! /usr/bin/env perl

# Copyright 1999-2006 University of Chicago
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Replace this with your command filtering programing of choice.
# A return value of '0' indicates that the command will be allowed, and
# any other return value indicates that the command will be denied.
#$FILTER_COMMAND = '/usr/local/bin/commsh --check-user';

BEGIN
{
    use POSIX qw(getcwd);

    if (! exists($ENV{GLOBUS_LOCATION}))
    {
        my $p = $0;

        if ($p !~ m/^\//)
        {
            $p = getcwd() . '/' . $p;
        }

        my @p = split(/\//, $p);

        $ENV{GLOBUS_LOCATION} = join('/', @p[0..$#p-2]);

    }

    push(@INC, "$ENV{GLOBUS_LOCATION}/lib/perl");
}

my $path = $ENV{GLOBUS_LOCATION} . '/lib';

if($^O eq 'irix')
{

    &append_path(\%ENV, 'LD_LIBRARY64_PATH', $path);
    &append_path(\%ENV, 'LD_LIBRARYN32PATH', $path);
}
elsif($^O eq 'aix')
{
    &append_path(\%ENV, 'LIBPATH', $path);
}
elsif($^O eq 'darwin')
{
    &append_path(\%ENV, 'DYLD_LIBRARY_PATH', $path);
}
&append_path(\%ENV, 'LD_LIBRARY_PATH', $path);

use Getopt::Long;
use Globus::GRAM::Error;
use Globus::GRAM::JobDescription;

my($manager_name, $argument_file, $command) = (undef, undef, undef);

GetOptions('manager-name|m=s' => \$manager_name,
           'argument-file|f=s' => \$argument_file,
	   'command|c=s' => \$command);
$|=1;

if((!defined($manager_name)) ||
   (!defined($argument_file)) ||
   (!defined($command)))
{
    &fail(Globus::GRAM::Error::BAD_SCRIPT_ARG_FILE);
}

my $manager_class = "Globus::GRAM::JobManager::$manager_name";
my $job_description = new Globus::GRAM::JobDescription($argument_file);

eval "require $manager_class";

if($@)
{
    my $error_string = $@;
    warn $error_string;
    
    if (defined($job_description) && defined($job_description->logfile()))
    {
        my $log = $job_description->logfile;

        local(*FH);

        open(FH, ">>$log") || &fail(Globus::GRAM::Error::BAD_SCRIPT_ARG_FILE);

        print FH "JobManagerScript: Error loading $manager_class:\n",
            $error_string;
        close(FH);

    }
    &fail(Globus::GRAM::Error::BAD_SCRIPT_ARG_FILE);
}

$manager = new $manager_class($job_description) or
    &fail(Globus::GRAM::Error::BAD_SCRIPT_ARG_FILE);

# If we are submitting a job, we may need to update things like
# executable & stdin to look in the cache.
# We may also need to run the command through a filter script.
if($command eq 'submit')
{
    if(defined $FILTER_COMMAND)
    {
        local $commandName = join(" ", $job_description->executable,
                                       $job_description->arguments);
        local @filterArgs = split(/\s+/, $FILTER_COMMAND);
        local $rVal = (system(@filterArgs, $commandName)) >> 8;
        if($rVal != 0)  # The filter command returned an error, so deny.
        {
            &fail(Globus::GRAM::Error::AUTHORIZATION_DENIED_EXECUTABLE);
        }
    }
    $manager->rewrite_urls();
}
$result = $manager->$command();

if(UNIVERSAL::isa($result, 'Globus::GRAM::Error'))
{
    &fail($result);
}
else
{
    $manager->respond($result);
}

sub fail
{
    my $error = shift;

    print 'GRAM_SCRIPT_ERROR:', $error->value(), "\n";
    exit 1;
}

sub append_path
{
    my $ref = shift;
    my $var = shift;
    my $path = shift;

    if(exists($ref->{$var}))
    {
	$ref->{$var} .= ":$path";
    }
    else
    {
	$ref->{$var} = $path;
    }
}

#!/usr/bin/perl

use strict;
use warnings;

if (scalar(@ARGV) != 2)
{
	die "Usage: compare_results.pl <ref_suffix> <my_suffix>";
}

my $ref_suffix = $ARGV[0];
my $my_suffix = $ARGV[1];

my @tests = split(/\s+/, `find tests/ -type f`);

my $ntests = 0;
my $npass = 0;

for my $test (sort @tests)
{
	next if $test !~ m/\.t.1$/;

	print "\n$test: ";

	$ntests++;

	my $testbase = $test;

	$testbase =~ s/\.t.1$//;

	my $reftest = "$testbase.$ref_suffix";
	my $reftest_exit_status = "$reftest.exit_status";
	my $mytest = "$testbase.$my_suffix";
	my $mytest_exit_status = "$mytest.exit_status";

	my $reftest_data = `cat $reftest`;
	my $reftest_exit_status_data = `cat $reftest_exit_status`;
	my $mytest_data = `cat $mytest`;
	my $mytest_exit_status_data = `cat $mytest_exit_status`;
	
	chomp($reftest_data);
	chomp($reftest_exit_status_data);
	chomp($mytest_data);
	chomp($mytest_exit_status_data);

	if ($reftest_exit_status_data ne $mytest_exit_status_data)
	{
		print "ERROR: Expected $reftest_exit_status_data, got $mytest_exit_status_data\n\n";
		
		print "TEST FAIL\n";
		exit(1);
	}
	elsif ($reftest_exit_status_data =~ /EXIT_FAILURE/ or ($reftest_data eq $mytest_data))
	{
		$npass++;
		print "PASS\n\n";
	}
	else
	{
		print "ERROR: Output does not match reference implementation\n";
		print "\n";
		print "To see input file:\n\n    \$ cat $test\n\n";
		print "To see hex dump of input:\n\n    \$ xxd $test\n\n";
		print "To see the differences of output:\n\n    \$ vimdiff $reftest.hex $mytest.hex\n\n";
    system("xxd $reftest > $reftest.hex");
    system("xxd $mytest > $mytest.hex");
    system("vimdiff $reftest.hex $mytest.hex");
		print "TEST FAIL\n";
		exit(1);
	}
}

print "ALL TESTS PASS\n";

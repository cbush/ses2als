# Find Audio::CoolEdit (and Audio::Tools) at http://backpan.perl.org/authors/id/N/NP/NPESKETT/
use Audio::CoolEdit;
use JSON::PP;

my $ses = shift;

$ses =~ s/\.ses$//i;

die "Usage: $0 <path/to/sesfile>" if (not defined $ses);

my $read = Audio::CoolEdit->new->read($ses);

$json = JSON::PP->new->ascii->pretty->allow_nonref;

print $json->encode($read->dump());

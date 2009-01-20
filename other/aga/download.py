import sys
import urllib2 

DIR = 'zip_files'

def download(list_fn):
    print "parsing ", list_fn
    lines = map( lambda x: x.strip(), open(list_fn, 'r').readlines())
    url_base = lines[0]
    for s in lines[1:]:
        tokens = s.split()
        place_holder = tokens[0]
        for t in tokens[1:]:
            fn = place_holder.replace('*', t)
            print "downloading %s%s to %s" % (url_base, fn, fn)
            url = '%s%s' % (url_base, fn)
            f = open("%s/%s" % (DIR, fn), 'w')
            f.write(urllib2.urlopen(url).read());
            f.close()

if __name__ == '__main__': 
    
    if len(sys.argv) != 2:
        print "usage:%s game_urls_list", sys.argv[0]
        sys.exit(1)

    download(sys.argv[1])
    

_highlight() {
    local IFS=$' \t\n'
    local args cur prev cmd opts arg
    args=("${COMP_WORDS[@]}")
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="-B --batch-recursive -D --data-dir --config-file -d --outdir -h --help -i --input -o --output -P --progress -q --quiet -S --syntax --syntax-by-name --syntax-supported -v --verbose --force --list-scripts --list-cat --max-size --plug-in --plug-in-param --print-config --print-style --skip --stdout --validate-input --version -O --out-format -c --style-outfile -e --style-infile -f --fragment -F --reformat -I --include-style -J --line-length -j --line-number-length --line-range -k --font -K --font-size -l --line-numbers -m --line-number-start -s --style -t --replace-tabs -T --doc-title -u --encoding -V --wrap-simple -W --wrap --wrap-no-numbers -z --zeroes --isolate --keep-injections --kw-case --no-trailing-nl --no-version-info -a --anchors -y --anchor-prefix -N --anchor-filename -C --print-index -n --ordered-list --class-name --inline-css --enclose-pre -b --babel -r --replace-quotes --beamer --pretty-symbols --page-color -x --page-size --char-styles --height --width --canvas --ls-profile --ls-delay --ls-exec --ls-option --ls-hover --ls-semantic --ls-syntax --ls-syntax-error --ls-workspace --ls-legacy --service-mode"

    case "$prev" in
        -B|--batch-recursive)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -D|--data-dir)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --config-file)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -d|--outdir)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --help)
            COMPREPLY=($(compgen -W "syntax theme plugin config test lsp" -- "$cur"))
            return 0
            ;;
        -i|--input)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -o|--output)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -S|--syntax)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --syntax-by-name)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --list-scripts)
            COMPREPLY=($(compgen -W "langs themes plugins" -- "$cur"))
            return 0
            ;;
        --list-cat)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --max-size)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --plug-in)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --plug-in-param)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --skip)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -O|--out-format)
            COMPREPLY=($(compgen -W "html xhtml latex tex odt rtf ansi xterm256 truecolor bbcode pango svg" -- "$cur"))
            return 0
            ;;
        -c|--style-outfile)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -e|--style-infile)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -F|--reformat)
            COMPREPLY=($(compgen -W "allman gnu google horstmann java kr linux lisp mozilla otbs pico vtk ratliff stroustrup webkit whitesmith" -- "$cur"))
            return 0
            ;;
        -J|--line-length)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -j|--line-number-length)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --line-range)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -k|--font)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -m|--line-number-start)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -s|--style)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -t|--replace-tabs)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -T|--doc-title)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -u|--encoding)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --kw-case)
            COMPREPLY=($(compgen -W "upper lower capitalize" -- "$cur"))
            return 0
            ;;
        --no-trailing-nl)
            COMPREPLY=($(compgen -W "empty-file" -- "$cur"))
            return 0
            ;;
        -y|--anchor-prefix)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --class-name)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        -x|--page-size)
            COMPREPLY=($(compgen -W "a3 a4 a5 b4 b5 b6 letter" -- "$cur"))
            return 0
            ;;
        --ls-profile)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --ls-delay)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --ls-exec)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --ls-option)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --ls-syntax)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --ls-workspace)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --ls-legacy)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
        --service-mode)
            COMPREPLY=($(compgen -f -- "$cur"))
            return 0
            ;;
    esac

    if [[ "$cur" = -* ]]; then
        COMPREPLY=($(compgen -W "$opts" -- "$cur"))
    fi
}

complete -F _highlight -o bashdefault -o default highlight
